#include "ds18b20.h"

static const char *TAG = "MAIN";

static void sampleTask(void *pvParameters) {
    set_gpio(5);
    while (1){
        int16_t temp;
        esp_err_t err;
        err = get_temperature (NULL, &temp);
        switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG,"current temp: %.1f\n", (float)(temp)/10.0f);
            break;
        case ESP_ERR_INVALID_CRC:
            ESP_LOGE(TAG,"CRC error");
            break;
        case ESP_ERR_NOT_FOUND:
            ESP_LOGW(TAG,"no DS18B20 detected");
            break;
       case ESP_ERR_TIMEOUT:
            ESP_LOGE(TAG,"DS18B20 timeout");
            break;
        default:
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    xTaskCreate(sampleTask, "SampleTask", 4096, NULL, 10, NULL);

}