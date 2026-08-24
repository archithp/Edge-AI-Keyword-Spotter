#include <stdio.h>
#include <string.h>

#include "model_data.h"
#include "inference.h"

#include "esp_log.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char *TAG = "ML";

static int8_t mfcc_int8[13][49];
extern float mfcc_out[13][49];
int prediction = 0;

namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
constexpr int kTensorArenaSize = 70 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
float scale = 0;
int zero = 0;
} 

void inference_init() {
    ESP_LOGI(TAG, "INITIALIZED");

    model = tflite::GetModel(g_model);

    static tflite::MicroMutableOpResolver<4> resolver;
    if (resolver.AddConv2D() != kTfLiteOk) {
        return;
    }
    if (resolver.AddMean() != kTfLiteOk) {
        return;
    }
    if (resolver.AddFullyConnected() != kTfLiteOk) {
        return;
    }
    if (resolver.AddSoftmax() != kTfLiteOk) {
        return;
    }

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        printf("Tensor allocation failed\n");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    scale = input->params.scale;
    zero = input->params.zero_point;
}

void inference_start(){
    ESP_LOGI(TAG, "Started inference");

    for (int i = 0; i < 13; i++){
        for (int j = 0; j < 49; j++){
            int32_t q = (int32_t)roundf(mfcc_out[i][j] / scale) + zero;
            if (q > 127)
                q = 127;
            else if (q < -128)
                q = -128;
            mfcc_int8[i][j] = (int8_t)q;
        }
    }

    memcpy(input->data.int8, mfcc_int8, MFCC_SIZE);

    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk){
        printf("Inference failed\n");
        return;
    }

    float max_score = -1000.0f;

    for (int i = 0; i < 4; i++){
        float score = (output->data.int8[i] - output->params.zero_point) * output->params.scale;
        if (score > max_score){
            max_score = score;
            prediction = i;
        }
    }
    ESP_LOGI(TAG, "Ended inference");
}