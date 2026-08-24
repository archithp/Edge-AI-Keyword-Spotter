#include "mqtt_client.h"
#include "esp_log.h"
#include "led_strip.h"

#define MQTT_BROKER "mqtt://industrial.api.ubidots.com:1883" 
#define MQTT_USERNAME "BBUS-QtEeEhZ52IT6N0LVFwECN01qhzXolo" 
#define MQTT_TOPIC "/v1.6/devices/esp32-s3"

#define LED_PIN GPIO_NUM_38
static led_strip_handle_t led_strip;

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;
extern int prediction;

static void mqtt_event_handler(void *arg,esp_event_base_t event_base,int32_t event_id,void *event_data)
{
    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG,"MQTT Connected");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG,"MQTT Disconnected");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG,"Message Published");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG,"MQTT Error");
            break;

        default:
            break;
    }
}


void mqtt_start(void)
{
    esp_mqtt_client_config_t cfg =
    {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = MQTT_USERNAME,
        .credentials.client_id = "esp32-s3-devkit"
    };


    client = esp_mqtt_client_init(&cfg);

    esp_mqtt_client_register_event(
        client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    esp_mqtt_client_start(client);
}


void mqtt_publish_yes(void)
{
    esp_mqtt_client_publish(client,MQTT_TOPIC,"{\"led\":0}",0, 1, 0);
    ESP_LOGI(TAG,"YES");
}


void mqtt_publish_no(void)
{
    esp_mqtt_client_publish(client,MQTT_TOPIC,"{\"led\":1}",0,1,0);
    ESP_LOGI(TAG,"NO");
}

void mqtt_publish_unknown(void)
{
    esp_mqtt_client_publish(client,MQTT_TOPIC,"{\"led\":2}",0, 1, 0);
    ESP_LOGI(TAG,"UNKNOWN");
}

void mqtt_publish_silence(void)
{
    esp_mqtt_client_publish(client,MQTT_TOPIC,"{\"led\":3}",0, 1, 0);
    ESP_LOGI(TAG,"SILENCE");
}

void mqtt_led(void){
    switch (prediction)
            {
                case 0:
                    mqtt_publish_yes();
                    printf("Keyword : YES\n");
                    led_strip_set_pixel(led_strip, 0, 16, 0, 0);
                    led_strip_refresh(led_strip);
                    break;
                case 1:
                    mqtt_publish_no();
                    printf("Keyword : NO\n");
                    led_strip_set_pixel(led_strip, 0, 0, 16, 0);
                    led_strip_refresh(led_strip);
                    break;
                case 2:
                    mqtt_publish_unknown();
                    printf("Keyword : UNKNOWN\n");
                    led_strip_set_pixel(led_strip, 0, 0, 0, 16);
                    led_strip_refresh(led_strip);
                    break;
                case 3:
                    mqtt_publish_silence();
                    printf("Keyword : SILENCE\n");
                    led_strip_set_pixel(led_strip, 0, 16, 16, 16);
                    led_strip_refresh(led_strip);
                    break;
                default:
                    printf("Keyword : DEFAULT\n");
                    led_strip_clear(led_strip);
                    break;
            }
}

void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = 1, // at least one LED on board
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}