#ifndef PENALTY_MODEL_H
#define PENALTY_MODEL_H

#include <stdio.h>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
class ONNX_Model{
    private:

    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    Ort::MemoryInfo memory_info;
    float renormalize_value(float value, float min, float max);

    public:
    ONNX_Model() : env(ORT_LOGGING_LEVEL_WARNING, "Inference"), session_options() ,session(Ort::Session(env, "model.onnx", session_options)), memory_info(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU)) {
        printf("Model loaded!\n");
    }

    std::vector<float> run_inference(std::vector<float> &input_tensor_values);
};

#endif
