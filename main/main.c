#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs_flash.h"
#include "esp_err.h"

#include "wifi.h"
#include "mqtt.h"
#include "i2s_mic.h"
#include "mfcc.h"
#include "inference.h"

//Queue Messages
typedef uint8_t audio_msg_t;
typedef uint8_t mfcc_msg_t;

typedef struct
{
    int prediction;
} result_msg_t;

//Queue Handles
QueueHandle_t audio_queue;
QueueHandle_t mfcc_queue;
QueueHandle_t result_queue;

//Global Variables
extern int16_t pcm_buffer[AUDIO_SAMPLES];
extern float mfcc_out[13][49];
extern int prediction;

// Audio Task
void audio_task(void *pvParameters){
    audio_msg_t msg = 1;
    while (1){
        /* Capture 1 second of audio */
        audio_capture();

        /* Notify DSP Task */
        xQueueSend(audio_queue, &msg, portMAX_DELAY);
    }
}

// DSP Task
void dsp_task(void *pvParameters){
    audio_msg_t msg;
    mfcc_msg_t mfcc_ready = 1;

    while (1){
        xQueueReceive(audio_queue,&msg,portMAX_DELAY);

        /* Convert PCM to MFCC */
        pcm_to_mfcc(pcm_buffer);

        /* Notify AI Task */
        xQueueSend(mfcc_queue,&mfcc_ready,portMAX_DELAY);
    }
}

// AI Task
void inference_task(void *pvParameters)
{
    mfcc_msg_t msg;
    result_msg_t result;

    while (1){
        xQueueReceive(mfcc_queue,&msg,portMAX_DELAY);
        inference_start();
        result.prediction = prediction;
        xQueueSend(result_queue,&result,portMAX_DELAY);
    }
}


//Output Task
void output_task(void *pvParameters){
    result_msg_t result;

    while (1){
        xQueueReceive(result_queue,&result,portMAX_DELAY);
        prediction = result.prediction;
        printf("Prediction = %d\n", prediction);
        mqtt_led();
    }
}


//app_main
void app_main(void)
{
    nvs_flash_init();
    wifi_init();
    configure_led();
    i2s_mic_init();
    mfcc_init();
    inference_init();
    mqtt_start();

    /* Create Queues */
    audio_queue = xQueueCreate(2, sizeof(audio_msg_t));
    mfcc_queue = xQueueCreate(2, sizeof(mfcc_msg_t));
    result_queue = xQueueCreate(2, sizeof(result_msg_t));

    if ((audio_queue == NULL) || (mfcc_queue == NULL) || (result_queue == NULL)){
        printf("Queue Creation Failed\n");
        return;
    }

    printf("Queues Created Successfully\n");

    /* Create Tasks */
    xTaskCreate(audio_task,"Audio Task",4096,NULL,5,NULL);
    xTaskCreate(dsp_task,"DSP Task",8192,NULL,4,NULL);
    xTaskCreate(inference_task,"AI Task",8192,NULL,4,NULL);
    xTaskCreate(output_task,"Output Task",4096,NULL,2,NULL);
}