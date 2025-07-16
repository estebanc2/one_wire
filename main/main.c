#include "ds18b20.h"

static const char *TAG = "MAIN";

#define SONDA_2  0x6F5558D446F86828
#define SONDA_3  0x942DA6D446B54928
#define SONDA_4  0xB24C96D4463F2E28
#define SONDA_5  0x0318F1D446673428
#define SONDA_6  0x2F05FAD446134328
#define SONDA_7  0x1A12BED446364B28
#define SONDA_8  0x15027CD446ECA728
#define SONDA_9  0x477716D4462FD328
#define SONDA_10 0xC96BD9D446C3BC28


#define ONE_WIRE_BUS 5


static uint64_t sonda[] = {
	SONDA_2,
	SONDA_3,
    SONDA_4,
    SONDA_5,
    SONDA_6,
    SONDA_7,
    SONDA_8,
    SONDA_9,
    SONDA_10
};

static size_t sonda_size = sizeof(sonda) / sizeof(sonda[0]);

static void sampleTask(void *pvParameters) {
    set_gpio(ONE_WIRE_BUS);
    int16_t temp[sonda_size];
    while (1){
        if( get_temperature (sonda, sonda_size, temp) == ESP_OK){
            for (uint8_t i = 0; i<(sonda_size); i++) {
                ESP_LOGI(TAG,"current sensor %d temperature = %.1f", i + 1, (float)(temp[i])/10.0f);
            }
        } else {    
            ESP_LOGW(TAG,"no DS18B20 detected");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
ESP_LOGI(TAG,"sonda size %d, value %016" PRIX64, sonda_size, sonda[0]);
//#define DISCOVER_ROM   
#ifdef DISCOVER_ROM
    uint64_t rom = 0;
    if (get_rom( 5, &rom) != ESP_OK){
        ESP_LOGW(TAG,"no DS18B20 detected");
    } else {
        ESP_LOGI(TAG,"current rom address sensor %016" PRIX64, rom);
    }
#else 
    xTaskCreate(sampleTask, "SampleTask", 2048, NULL, 10, NULL);
#endif
}