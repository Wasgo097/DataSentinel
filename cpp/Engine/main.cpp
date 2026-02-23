#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <filesystem>
#include <fstream>

#include <boost/asio.hpp>
// ONNX Runtime C++ API
#include <onnxruntime_cxx_api.h>

// include header-only nlohmann::json
#include "json.hpp"

using boost::asio::ip::tcp;
std::vector<double> parse_input(const std::string &input)
{
    std::vector<double> values;
    std::stringstream ss(input);
    double value;

    while (ss >> value)
        values.push_back(value);

    return values;
}

// Load threshold from fixed config.json
double load_threshold_from_json()
{
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    fs::path config_path = fs::absolute("models/config.json");

    if (!fs::exists(config_path))
    {
        throw std::runtime_error("Config file not found at " + config_path.string());
    }

    try
    {
        std::ifstream ifs(config_path);
        if (!ifs.is_open())
        {
            throw std::runtime_error("Failed to open config file at " + config_path.string());
        }

        json j;
        ifs >> j;

        if (j.contains("threshold") && j["threshold"].is_number())
            return j["threshold"].get<double>();
        throw std::runtime_error("'threshold' key missing or not a number in config.json");
    }
    catch (const std::exception &e)
    {
        std::cerr << "[WARN] Failed to parse config.json: " << e.what();
    }

    throw std::runtime_error("No threshold value found in config.json");
}

// ------------------------
// Load ONNX model from fixed path
// Throws if model file does not exist
// ------------------------
Ort::Session load_model(Ort::Env &env, Ort::SessionOptions &session_options)
{
    namespace fs = std::filesystem;
    fs::path model_path = fs::absolute("models/model.onnx");

    if (!fs::exists(model_path))
        throw std::runtime_error("Model file not found at " + model_path.string());

    return Ort::Session(env, model_path.c_str(), session_options);
}

// ------------------------
// Validate input size against model expectations
// Returns true if valid, false otherwise (already sends error response to socket)
// ------------------------
bool validate_input(const Ort::Session &session, const std::vector<double> &values, tcp::socket &socket)
{
    try
    {
        auto input_type_info = session.GetInputTypeInfo(0);
        auto tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        auto shape = tensor_info.GetShape();

        // Model shape is [1, num_features], check second dimension
        int64_t expected_size = (shape.size() > 1) ? shape[1] : shape[0];

        if (static_cast<int64_t>(values.size()) != expected_size)
        {
            std::cerr << "[ERROR] Input size mismatch. Expected: " << expected_size
                      << ", Got: " << values.size() << "\n";

            std::string error_response = "ERROR: Invalid input size\n";
            boost::asio::write(socket, boost::asio::buffer(error_response));
            return false;
        }
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Failed to validate input: " << e.what() << "\n";
        std::string error_response = "ERROR: Input validation failed\n";
        boost::asio::write(socket, boost::asio::buffer(error_response));
        return false;
    }
}

// ------------------------
// Main
// ------------------------
int main()
{
    try
    {
        // Initialize ONNX Runtime environment and session options
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "DataSentinel");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        // Load model (throws if missing)
        Ort::Session session = load_model(env, session_options);

        // Load threshold
        double THRESHOLD = load_threshold_from_json();
        std::cout << "Using threshold: " << THRESHOLD << "\n";

        // Start TCP server
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9000));
        std::cout << "TCP Server listening on port 9000...\n";

        Ort::AllocatorWithDefaultOptions allocator;

        while (true)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::cout << "Client connected.\n";

            // Handle multiple messages from the same connection
            while (socket.is_open())
            {
                try
                {
                    boost::asio::streambuf buffer;
                    boost::asio::read_until(socket, buffer, "\n");

                    std::istream input_stream(&buffer);
                    std::string data;
                    std::getline(input_stream, data);

                    std::cout << "Received: " << data << "\n";

                    auto values = parse_input(data);

                    // Validate input
                    if (!validate_input(session, values, socket))
                    {
                        continue;
                    }

                    double score = 0.0;
                    try
                    {
                        // Prepare input tensor: assume model expects 1xN float tensor
                        std::vector<float> input_tensor_values;
                        input_tensor_values.reserve(values.size());
                        for (double v : values)
                            input_tensor_values.push_back(static_cast<float>(v));

                        std::vector<int64_t> input_shape = {1, static_cast<int64_t>(values.size())};

                        std::vector<std::string> input_name_vec = session.GetInputNames();
                        std::vector<std::string> output_name_vec = session.GetOutputNames();

                        if (input_name_vec.empty() || output_name_vec.empty())
                            throw std::runtime_error("Model has no input or output names");

                        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
                        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                            memory_info,
                            input_tensor_values.data(),
                            input_tensor_values.size(),
                            input_shape.data(),
                            input_shape.size());

                        const char *input_names[] = {input_name_vec[0].c_str()};
                        const char *output_names[] = {output_name_vec[0].c_str()};

                        auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

                        float *float_array = output_tensors[0].GetTensorMutableData<float>();
                        score = static_cast<double>(float_array[0]);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "ONNX inference failed: " << e.what() << "\n";
                    }

                    std::string response = (score > THRESHOLD) ? "ANOMALY\n" : "OK\n";
                    boost::asio::write(socket, boost::asio::buffer(response));

                    std::cout << "Response sent: " << response;
                }
                catch (const boost::system::system_error &e)
                {
                    // Handle EOF, connection reset, and other system errors
                    if (e.code() == boost::asio::error::eof)
                    {
                        std::cout << "Client disconnected gracefully.\n";
                    }
                    else if (e.code() == boost::asio::error::connection_reset)
                    {
                        std::cout << "Client connection reset.\n";
                    }
                    else
                    {
                        std::cerr << "[ERROR] Connection error: " << e.what() << "\n";
                    }
                    break;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "[ERROR] Unexpected error: " << e.what() << "\n";
                    break;
                }
            }

            if (socket.is_open())
            {
                socket.close();
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}