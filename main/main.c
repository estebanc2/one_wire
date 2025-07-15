#include "ds18b20.h"

static const char *TAG = "MAIN";

#define SONDA_1A 0xB56ED3D44648D828
#define SONDA_2A 0xC36522D446883828
#define SONDA_3A 0xF26D24D446F51A28
#define SONDA_4A 0x6F1FFFD446070E28
#define SONDA_5A 0xE30AACD446FB7028
#define SONDA_6A 0x58705AD446BE6228
#define SONDA_7A 0x1D40A6D446374A28
#define SONDA_8A 0xA5572DD446CFE828
#define SONDA_9A 0x4725B0D446927728


#define ONE_WIRE_BUS 5


static uint64_t sonda[] = {
	SONDA_1A,
	SONDA_2A,
	SONDA_3A,
    SONDA_4A,
    SONDA_5A,
    SONDA_6A,
    SONDA_7A,
    SONDA_8A,
    SONDA_9A
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