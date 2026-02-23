#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <boost/asio.hpp>
#include <onnxruntime_cxx_api.h>
#include "json.hpp"

using boost::asio::ip::tcp;
using json = nlohmann::json;

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////
// =================== LOGGING ==============================
//////////////////////////////////////////////////////////////

constexpr bool DEBUG_LOG = true;

void log_info(const std::string &msg)
{
    if (DEBUG_LOG)
        std::cout << "[INFO] " << msg << std::endl;
}

void log_error(const std::string &msg)
{
    std::cerr << "[ERROR] " << msg << std::endl;
}

//////////////////////////////////////////////////////////////
// =================== PARSING ==============================
//////////////////////////////////////////////////////////////

std::vector<double> parse_input(const std::string &input)
{
    std::vector<double> values;
    std::stringstream ss(input);
    double value;

    while (ss >> value)
        values.push_back(value);

    return values;
}

//////////////////////////////////////////////////////////////
// =================== CONFIG ===============================
//////////////////////////////////////////////////////////////

double load_threshold(const std::string &path)
{
    fs::path config_path = fs::absolute(path);

    if (!fs::exists(config_path))
        throw std::runtime_error("Config file not found: " + config_path.string());

    std::ifstream ifs(config_path);
    if (!ifs.is_open())
        throw std::runtime_error("Failed to open config file");

    json j;
    ifs >> j;

    if (!j.contains("threshold") || !j["threshold"].is_number())
        throw std::runtime_error("Invalid or missing 'threshold'");

    return j["threshold"].get<double>();
}

//////////////////////////////////////////////////////////////
// =================== MODEL INIT ===========================
//////////////////////////////////////////////////////////////

Ort::Session create_session(Ort::Env &env,
                            const std::string &model_path)
{
    fs::path path = fs::absolute(model_path);

    if (!fs::exists(path))
        throw std::runtime_error("Model not found: " + path.string());

    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_BASIC);

    return Ort::Session(env, path.c_str(), options);
}

int64_t get_expected_input_size(Ort::Session &session)
{
    auto type_info = session.GetInputTypeInfo(0);

    if (type_info.GetONNXType() != ONNX_TYPE_TENSOR)
        throw std::runtime_error("Model input is not a tensor");

    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    auto shape = tensor_info.GetShape();

    if (shape.empty())
        throw std::runtime_error("Input tensor has no shape");

    return (shape.size() > 1) ? shape[1] : shape[0];
}

//////////////////////////////////////////////////////////////
// =================== INFERENCE ============================
//////////////////////////////////////////////////////////////

double run_inference(Ort::Session &session,
                     const Ort::MemoryInfo &memory_info,
                     const std::vector<const char *> &input_names,
                     const std::vector<const char *> &output_names,
                     int64_t expected_size,
                     const std::vector<double> &values)
{
    std::vector<float> tensor_values;
    tensor_values.reserve(values.size());

    for (double v : values)
        tensor_values.push_back(static_cast<float>(v));

    std::vector<int64_t> shape = {1, expected_size};

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        tensor_values.data(),
        tensor_values.size(),
        shape.data(),
        shape.size());

    auto output_tensors = session.Run(
        Ort::RunOptions{nullptr},
        input_names.data(),
        &input_tensor,
        1,
        output_names.data(),
        1);

    float *output = output_tensors[0].GetTensorMutableData<float>();
    return static_cast<double>(output[0]);
}

//////////////////////////////////////////////////////////////
// =================== CLIENT HANDLING ======================
//////////////////////////////////////////////////////////////

void handle_client(tcp::socket &socket,
                   Ort::Session &session,
                   const Ort::MemoryInfo &memory_info,
                   const std::vector<const char *> &input_names,
                   const std::vector<const char *> &output_names,
                   int64_t expected_size,
                   double threshold)
{
    log_info("Client connected: " +
             socket.remote_endpoint().address().to_string());

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

            log_info("Received raw: " + data);

            auto values = parse_input(data);

            log_info("Parsed values count: " +
                     std::to_string(values.size()));

            if (static_cast<int64_t>(values.size()) != expected_size)
            {
                log_error("Invalid input size");

                std::string response = "ERROR: Invalid input size\n";
                boost::asio::write(socket,
                                   boost::asio::buffer(response));

                continue;
            }

            double score = run_inference(
                session,
                memory_info,
                input_names,
                output_names,
                expected_size,
                values);

            log_info("Model score: " + std::to_string(score));

            std::string response =
                (score > threshold) ? "ANOMALY\n" : "OK\n";

            log_info("Sending response: " + response);

            boost::asio::write(socket,
                               boost::asio::buffer(response));
        }
        catch (const std::exception &e)
        {
            log_error(e.what());
            break;
        }
    }

    if (socket.is_open())
        socket.close();

    log_info("Client disconnected");
}

//////////////////////////////////////////////////////////////
// =================== MAIN ================================
//////////////////////////////////////////////////////////////

int main()
{
    try
    {
        // ---- INIT MODEL ----
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "DataSentinel");
        Ort::Session session =
            create_session(env, "models/model.onnx");

        int64_t expected_size =
            get_expected_input_size(session);

        Ort::MemoryInfo memory_info =
            Ort::MemoryInfo::CreateCpu(
                OrtArenaAllocator,
                OrtMemTypeDefault);

        // ---- INPUT / OUTPUT NAMES ----
        std::vector<std::string> input_names_str =
            session.GetInputNames();

        std::vector<std::string> output_names_str =
            session.GetOutputNames();

        std::vector<const char *> input_names;
        std::vector<const char *> output_names;

        for (auto &s : input_names_str)
            input_names.push_back(s.c_str());

        for (auto &s : output_names_str)
            output_names.push_back(s.c_str());

        // ---- CONFIG ----
        double threshold =
            load_threshold("models/config.json");

        std::cout << "Threshold: "
                  << threshold << "\n";

        // ---- TCP SERVER ----
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(
            io_context,
            tcp::endpoint(tcp::v4(), 9000));

        std::cout
            << "Server listening on port 9000...\n";

        while (true)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            handle_client(socket,
                          session,
                          memory_info,
                          input_names,
                          output_names,
                          expected_size,
                          threshold);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: "
                  << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}