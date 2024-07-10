#include "ONNX_model.h"

std::vector<float> ONNX_Model::run_inference(std::vector<float> &input_tensor_values) {
        std::vector<int64_t> input_tensor_shape = {1, 9};
        std::vector<int64_t> output_tensor_shape = {1, 2};
        std::vector<float> output_tensor_values(2);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_tensor_shape.data(), input_tensor_shape.size());
        Ort::Value output_tensor = Ort::Value::CreateTensor<float>(memory_info, output_tensor_values.data(), output_tensor_values.size(), output_tensor_shape.data(), output_tensor_shape.size());
        const char* input_names[] = {"input"};
        const char* output_names[] = {"33"};
        Ort::RunOptions run_options;
        session.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
        printf("Inference done!\n");
        output_tensor_values[0] = renormalize_value(output_tensor_values[0], -1, 1);
        output_tensor_values[1] = renormalize_value(output_tensor_values[1], 0.15, 1);
        return output_tensor_values;
    }

float ONNX_Model::renormalize_value(float value, float min, float max) {
        float normalized = (value + 1) /2.0;
        float action_mapped = normalized * (max - min) + min;
        return action_mapped;
    }