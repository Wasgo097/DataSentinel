#include "TensorRtInferenceBackend.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "Logger.hpp"
#include "TensorRtBuildOptions.hpp"
#include "TensorRtEngineStore.hpp"

#if DS_ENABLE_TENSORRT
#include <cuda_runtime_api.h>
#include <NvInfer.h>
#endif

namespace ds
{
#if DS_ENABLE_TENSORRT
namespace
{
std::size_t resolve_expected_input_size_from_onnx(const std::filesystem::path &onnx_path)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "DataSentinelTensorRtInputShape");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

    Ort::Session session(env, onnx_path.c_str(), options);

    const auto input_type_info = session.GetInputTypeInfo(0);
    if (input_type_info.GetONNXType() != ONNX_TYPE_TENSOR)
    {
        throw std::runtime_error("TensorRT backend expects tensor input, but ONNX input type is different");
    }

    const auto tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
    const auto shape = tensor_info.GetShape();
    if (shape.empty())
    {
        throw std::runtime_error("ONNX input tensor has empty shape");
    }

    const int64_t feature_dim = (shape.size() > 1) ? shape[1] : shape[0];
    if (feature_dim <= 0)
    {
        throw std::runtime_error("ONNX input tensor has non-static or invalid feature dimension");
    }

    return static_cast<std::size_t>(feature_dim);
}

std::size_t volume_of_dims(const nvinfer1::Dims &dims)
{
    std::size_t volume = 1;
    for (int i = 0; i < dims.nbDims; ++i)
    {
        if (dims.d[i] <= 0)
        {
            throw std::runtime_error("TensorRT tensor shape has dynamic or invalid dimensions");
        }
        volume *= static_cast<std::size_t>(dims.d[i]);
    }
    return volume;
}

void throw_if_cuda_failed(cudaError_t code, const std::string &context)
{
    if (code != cudaSuccess)
    {
        throw std::runtime_error(context + ": " + cudaGetErrorString(code));
    }
}

class TensorRtLogger final : public nvinfer1::ILogger
{
public:
    void log(Severity severity, const char *msg) noexcept override
    {
        if (severity <= Severity::kWARNING)
        {
            std::cerr << "[TensorRT] " << msg << '\n';
        }
    }
};

TensorRtLogger &trt_logger()
{
    static TensorRtLogger logger;
    return logger;
}
} // namespace

class TensorRtInferenceBackend::Impl
{
public:
    Impl(const std::filesystem::path &engine_path, std::size_t expected_input_size)
        : expected_input_size_(expected_input_size)
    {
        load_engine_bytes(engine_path);

        runtime_.reset(nvinfer1::createInferRuntime(trt_logger()));
        if (!runtime_)
        {
            throw std::runtime_error("Failed to create TensorRT runtime");
        }

        engine_.reset(runtime_->deserializeCudaEngine(engine_bytes_.data(), engine_bytes_.size()));
        if (!engine_)
        {
            throw std::runtime_error("Failed to deserialize TensorRT engine from: " + engine_path.string());
        }

        context_.reset(engine_->createExecutionContext());
        if (!context_)
        {
            throw std::runtime_error("Failed to create TensorRT execution context");
        }

        resolve_tensor_names();
        validate_tensor_types();
        configure_tensor_shapes();
        allocate_device_buffers();

        throw_if_cuda_failed(cudaStreamCreate(&stream_), "Failed to create CUDA stream");
    }

    ~Impl()
    {
        if (device_input_ != nullptr)
        {
            cudaFree(device_input_);
        }
        if (device_output_ != nullptr)
        {
            cudaFree(device_output_);
        }
        if (stream_ != nullptr)
        {
            cudaStreamDestroy(stream_);
        }
    }

    std::vector<float> reconstruct(const std::vector<float> &input)
    {
        if (input.size() != expected_input_size_)
        {
            throw std::runtime_error("Invalid input size for TensorRT backend");
        }

        throw_if_cuda_failed(
            cudaMemcpyAsync(device_input_,
                            input.data(),
                            input_bytes_,
                            cudaMemcpyHostToDevice,
                            stream_),
            "Failed to copy TensorRT input to device");

        if (!context_->setTensorAddress(input_name_.c_str(), device_input_))
        {
            throw std::runtime_error("Failed to bind TensorRT input tensor address");
        }

        if (!context_->setTensorAddress(output_name_.c_str(), device_output_))
        {
            throw std::runtime_error("Failed to bind TensorRT output tensor address");
        }

        if (!context_->enqueueV3(stream_))
        {
            throw std::runtime_error("TensorRT inference enqueue failed");
        }

        std::vector<float> output(output_elements_);
        throw_if_cuda_failed(
            cudaMemcpyAsync(output.data(),
                            device_output_,
                            output_bytes_,
                            cudaMemcpyDeviceToHost,
                            stream_),
            "Failed to copy TensorRT output to host");

        throw_if_cuda_failed(cudaStreamSynchronize(stream_), "Failed to synchronize CUDA stream");

        if (output.size() < expected_input_size_)
        {
            throw std::runtime_error("TensorRT output is smaller than expected input size");
        }

        output.resize(expected_input_size_);
        return output;
    }

private:
    struct InferDeleter
    {
        template <typename T>
        void operator()(T *ptr) const
        {
            delete ptr;
        }
    };

    using RuntimePtr = std::unique_ptr<nvinfer1::IRuntime, InferDeleter>;
    using EnginePtr = std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter>;
    using ContextPtr = std::unique_ptr<nvinfer1::IExecutionContext, InferDeleter>;

    void load_engine_bytes(const std::filesystem::path &engine_path)
    {
        std::ifstream input(engine_path, std::ios::binary | std::ios::ate);
        if (!input.is_open())
        {
            throw std::runtime_error("Failed to open TensorRT engine file: " + engine_path.string());
        }

        const auto end_pos = input.tellg();
        if (end_pos <= 0)
        {
            throw std::runtime_error("TensorRT engine file is empty: " + engine_path.string());
        }

        engine_bytes_.resize(static_cast<std::size_t>(end_pos));
        input.seekg(0, std::ios::beg);
        input.read(engine_bytes_.data(), static_cast<std::streamsize>(engine_bytes_.size()));

        if (!input.good())
        {
            throw std::runtime_error("Failed to read TensorRT engine file: " + engine_path.string());
        }
    }

    void resolve_tensor_names()
    {
        const int tensor_count = engine_->getNbIOTensors();
        for (int i = 0; i < tensor_count; ++i)
        {
            const char *name = engine_->getIOTensorName(i);
            if (name == nullptr)
            {
                continue;
            }

            const auto mode = engine_->getTensorIOMode(name);
            if (mode == nvinfer1::TensorIOMode::kINPUT && input_name_.empty())
            {
                input_name_ = name;
            }
            else if (mode == nvinfer1::TensorIOMode::kOUTPUT && output_name_.empty())
            {
                output_name_ = name;
            }
        }

        if (input_name_.empty() || output_name_.empty())
        {
            throw std::runtime_error("TensorRT engine must expose one input and one output tensor");
        }
    }

    void validate_tensor_types() const
    {
        if (engine_->getTensorDataType(input_name_.c_str()) != nvinfer1::DataType::kFLOAT)
        {
            throw std::runtime_error("TensorRT input tensor type must be float32");
        }

        if (engine_->getTensorDataType(output_name_.c_str()) != nvinfer1::DataType::kFLOAT)
        {
            throw std::runtime_error("TensorRT output tensor type must be float32");
        }
    }

    void configure_tensor_shapes()
    {
        nvinfer1::Dims input_dims = engine_->getTensorShape(input_name_.c_str());

        if (input_dims.nbDims == 2)
        {
            input_dims.d[0] = 1;
            input_dims.d[1] = static_cast<int>(expected_input_size_);
        }
        else if (input_dims.nbDims == 1)
        {
            input_dims.d[0] = static_cast<int>(expected_input_size_);
        }

        if (!context_->setInputShape(input_name_.c_str(), input_dims))
        {
            throw std::runtime_error("Failed to set TensorRT input shape");
        }

        const nvinfer1::Dims resolved_input_dims = context_->getTensorShape(input_name_.c_str());
        const nvinfer1::Dims resolved_output_dims = context_->getTensorShape(output_name_.c_str());

        input_elements_ = volume_of_dims(resolved_input_dims);
        output_elements_ = volume_of_dims(resolved_output_dims);

        if (input_elements_ < expected_input_size_)
        {
            throw std::runtime_error("TensorRT input tensor has fewer elements than expected");
        }

        input_bytes_ = input_elements_ * sizeof(float);
        output_bytes_ = output_elements_ * sizeof(float);
    }

    void allocate_device_buffers()
    {
        throw_if_cuda_failed(cudaMalloc(&device_input_, input_bytes_), "Failed to allocate TensorRT input buffer");

        try
        {
            throw_if_cuda_failed(cudaMalloc(&device_output_, output_bytes_), "Failed to allocate TensorRT output buffer");
        }
        catch (...)
        {
            cudaFree(device_input_);
            device_input_ = nullptr;
            throw;
        }
    }

    std::size_t expected_input_size_{0};
    std::vector<char> engine_bytes_;

    RuntimePtr runtime_{nullptr};
    EnginePtr engine_{nullptr};
    ContextPtr context_{nullptr};

    std::string input_name_;
    std::string output_name_;

    std::size_t input_elements_{0};
    std::size_t output_elements_{0};
    std::size_t input_bytes_{0};
    std::size_t output_bytes_{0};

    void *device_input_{nullptr};
    void *device_output_{nullptr};
    cudaStream_t stream_{nullptr};
};
#else
class TensorRtInferenceBackend::Impl
{
};
#endif

TensorRtInferenceBackend::TensorRtInferenceBackend(const std::string &model_path)
    : model_path_(std::filesystem::absolute(model_path))
{
#if DS_ENABLE_TENSORRT
    if (!std::filesystem::exists(model_path_))
    {
        throw std::runtime_error("Model file not found: " + model_path_.string());
    }

    TensorRtBuildOptions build_options{};
    TensorRtEngineBuilder builder(build_options);
    TensorRtEngineStore store(std::move(builder));
    engine_path_ = store.ensure_engine_file(model_path_);

    expected_input_size_ = resolve_expected_input_size_from_onnx(model_path_);
    impl_ = std::make_unique<Impl>(engine_path_, expected_input_size_);

    ds::log::info("TensorRT engine file: " + engine_path_.string());
#else
    throw std::runtime_error("TensorRT backend is unavailable because binary was built without TensorRT support");
#endif
}

TensorRtInferenceBackend::~TensorRtInferenceBackend() = default;

TensorRtInferenceBackend::TensorRtInferenceBackend(TensorRtInferenceBackend &&) noexcept = default;

TensorRtInferenceBackend &TensorRtInferenceBackend::operator=(TensorRtInferenceBackend &&) noexcept = default;

std::string TensorRtInferenceBackend::backend_name() const
{
    return "tensorrt";
}

std::size_t TensorRtInferenceBackend::expected_input_size() const
{
    return expected_input_size_;
}

std::vector<float> TensorRtInferenceBackend::reconstruct(const std::vector<float> &input)
{
#if DS_ENABLE_TENSORRT
    if (!impl_)
    {
        throw std::runtime_error("TensorRT backend internal state is not initialized");
    }
    return impl_->reconstruct(input);
#else
    (void)input;
    throw std::runtime_error("TensorRT backend is unavailable because binary was built without TensorRT support");
#endif
}
} // namespace ds
