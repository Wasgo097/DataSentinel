#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <filesystem>
#include <fstream>

#include <boost/asio.hpp>
#include <onnxruntime_cxx_api.h>
#include "json.hpp"

using boost::asio::ip::tcp;

// ========================
// Parse input string into vector of doubles
// ========================
std::vector<double> parse_input(const std::string &input)
{
    std::vector<double> values;
    std::stringstream ss(input);
    double value;

    while (ss >> value)
        values.push_back(value);

    return values;
}

// ========================
// Load threshold from config file
// ========================
double load_threshold_from_json()
{
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    fs::path config_path = fs::absolute("models/config.json");

    if (!fs::exists(config_path))
        throw std::runtime_error("Config file not found at " + config_path.string());

    std::ifstream ifs(config_path);
    if (!ifs.is_open())
        throw std::runtime_error("Failed to open config file at " + config_path.string());

    json j;
    ifs >> j;

    if (j.contains("threshold") && j["threshold"].is_number())
        return j["threshold"].get<double>();
    throw std::runtime_error("'threshold' key missing or not a number in config.json");
}

// ========================
// Load ONNX model from fixed path
// ========================
Ort::Session load_model(Ort::Env &env, Ort::SessionOptions &session_options)
{
    namespace fs = std::filesystem;
    fs::path model_path = fs::absolute("models/model.onnx");

    if (!fs::exists(model_path))
        throw std::runtime_error("Model file not found at " + model_path.string());

    return Ort::Session(env, model_path.c_str(), session_options);
}

// ========================
// Validate input size against model expectations
// ========================
bool validate_input(int64_t expected_size,
                    const std::vector<double> &values,
                    tcp::socket &socket)
{
    if (static_cast<int64_t>(values.size()) != expected_size)
    {
        std::cerr << "Input size mismatch. Expected: "
                  << expected_size
                  << ", Got: "
                  << values.size()
                  << "\n";

        std::string error_response = "ERROR: Invalid input size\n";
        boost::asio::write(socket, boost::asio::buffer(error_response));
        return false;
    }

    return true;
}

// ========================
// Run ONNX model inference
// ========================
double run_inference(Ort::Session &session,
                     const Ort::MemoryInfo &memory_info,
                     std::vector<float> &input_tensor_values,
                     const std::vector<double> &values,
                     const char *const *input_names,
                     const char *const *output_names)
{
    input_tensor_values.clear();
    input_tensor_values.reserve(values.size());

    for (double v : values)
        input_tensor_values.push_back(static_cast<float>(v));

    std::vector<int64_t> input_shape = {
        1,
        static_cast<int64_t>(values.size())
    };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_tensor_values.data(),
        input_tensor_values.size(),
        input_shape.data(),
        input_shape.size()
    );

    auto output_tensors = session.Run(
        Ort::RunOptions{nullptr},
        input_names,
        &input_tensor,
        1,
        output_names,
        1
    );

    float *float_array = output_tensors[0].GetTensorMutableData<float>();
    return static_cast<double>(float_array[0]);
}

// ========================
// Process a single client message
// ========================
bool process_message(Ort::Session &session,
                     const Ort::MemoryInfo &memory_info,
                     int64_t expected_size,
                     std::vector<float> &input_tensor_values,
                     const char *const *input_names,
                     const char *const *output_names,
                     double threshold,
                     const std::string &data,
                     tcp::socket &socket)
{
    try
    {
        std::cout << "Received: " << data << "\n";

        auto values = parse_input(data);

        if (!validate_input(expected_size, values, socket))
            return false;

        double score = run_inference(
            session,
            memory_info,
            input_tensor_values,
            values,
            input_names,
            output_names
        );

        std::string response =
            (score > threshold) ? "ANOMALY\n" : "OK\n";

        boost::asio::write(socket, boost::asio::buffer(response));

        std::cout << "Response sent: " << response;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ONNX inference failed: "
                  << e.what()
                  << "\n";

        std::string error_response =
            "ERROR: Inference failed\n";

        try
        {
            boost::asio::write(socket,
                               boost::asio::buffer(error_response));
        }
        catch (...)
        {
            // Client may have disconnected
        }

        return false;
    }
}

// ========================
// Handle client connection
// ========================
void handle_client(Ort::Session &session,
                   const Ort::MemoryInfo &memory_info,
                   int64_t expected_size,
                   std::vector<float> &input_tensor_values,
                   const char *const *input_names,
                   const char *const *output_names,
                   double threshold,
                   tcp::socket &socket)
{
    std::cout << "Client connected.\n";

    boost::asio::streambuf buffer;

    while (socket.is_open())
    {
        try
        {
            buffer.consume(buffer.size());
            boost::asio::read_until(socket, buffer, "\n");

            std::istream input_stream(&buffer);
            std::string data;
            std::getline(input_stream, data);

            process_message(session,
                            memory_info,
                            expected_size,
                            input_tensor_values,
                            input_names,
                            output_names,
                            threshold,
                            data,
                            socket);
        }
        catch (const boost::system::system_error &e)
        {
            if (e.code() == boost::asio::error::eof)
                std::cout << "Client disconnected gracefully.\n";
            else if (e.code() == boost::asio::error::connection_reset)
                std::cout << "Client connection reset.\n";
            else
                std::cerr << "Connection error: "
                          << e.what()
                          << "\n";

            break;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Unexpected error: "
                      << e.what()
                      << "\n";
            break;
        }
    }

    if (socket.is_open())
        socket.close();
}

// ========================
// Main
// ========================
int main()
{
    try
    {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "DataSentinel");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_BASIC
        );

        Ort::Session session =
            load_model(env, session_options);

        double THRESHOLD =
            load_threshold_from_json();

        std::cout << "Using threshold: "
                  << THRESHOLD
                  << "\n";

        std::vector<std::string> input_name_vec =
            session.GetInputNames();
        std::vector<std::string> output_name_vec =
            session.GetOutputNames();

        if (input_name_vec.empty() ||
            output_name_vec.empty())
        {
            throw std::runtime_error(
                "Model has no input or output names"
            );
        }

        std::vector<const char*> input_names_c;
        std::vector<const char*> output_names_c;

        for (auto &s : input_name_vec)
            input_names_c.push_back(s.c_str());

        for (auto &s : output_name_vec)
            output_names_c.push_back(s.c_str());

        auto type_info =
            session.GetInputTypeInfo(0);

        if (type_info.GetONNXType() != ONNX_TYPE_TENSOR)
            throw std::runtime_error(
                "Input is not a tensor!"
            );

        auto tensor_info =
            type_info.GetTensorTypeAndShapeInfo();

        auto shape = tensor_info.GetShape();

        if (shape.empty())
            throw std::runtime_error(
                "Input tensor has no shape information"
            );

        int64_t expected_size =
            (shape.size() > 1) ?
            shape[1] :
            shape[0];

        std::vector<float> input_tensor_values;
        input_tensor_values.reserve(expected_size);

        Ort::MemoryInfo memory_info =
            Ort::MemoryInfo::CreateCpu(
                OrtArenaAllocator,
                OrtMemTypeDefault
            );

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(
            io_context,
            tcp::endpoint(tcp::v4(), 9000)
        );

        std::cout
            << "TCP Server listening on port 9000...\n";

        while (true)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            handle_client(session,
                          memory_info,
                          expected_size,
                          input_tensor_values,
                          input_names_c.data(),
                          output_names_c.data(),
                          THRESHOLD,
                          socket);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: "
                  << e.what()
                  << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}