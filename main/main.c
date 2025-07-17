#include "ds18b20.h"

#define LOOP_PERIOD 2   // seconds
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
/*
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
*/
static size_t sonda_size = sizeof(sonda) / sizeof(sonda[0]);

static void sampleTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_PERIOD * 1000); // N segundos
    
    // Mejor: usa malloc para evitar stack overflow
    int16_t *temp = malloc(sonda_size * sizeof(int16_t));
    if (!temp) {
        ESP_LOGE(TAG, "No memory for temp array");
        vTaskDelete(NULL);
        return;
    }
   
    while (true){
        TickType_t start_time = xTaskGetTickCount();
        if( get_temperature (sonda, sonda_size, temp) == ESP_OK){
            for (uint8_t i = 0; i<(sonda_size); i++) {
                ESP_LOGI(TAG,"current sensor %d temperature = %.1f", i + 2, (float)(temp[i])/100.0f);
            }
        } else {    
            ESP_LOGW(TAG,"no DS18B20 detected");
        }
        TickType_t elapsed_time = xTaskGetTickCount() - start_time;
        if (elapsed_time > xFrequency) {
            ESP_LOGW(TAG, "Loop took longer than period! (%lu ms > %lu ms)", 
                    pdTICKS_TO_MS(elapsed_time), 
                    pdTICKS_TO_MS(xFrequency));
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    free(temp);
    vTaskDelete(NULL);
    ESP_LOGE(TAG,"algo malo paso porque se esta cerrando la task");
}

void app_main(void) {
//#define DISCOVER_ROM   
#ifdef DISCOVER_ROM
    uint64_t rom = 0;
    if (get_rom( 5, &rom) != ESP_OK){
        ESP_LOGW(TAG,"no DS18B20 detected");
    } else {
        ESP_LOGI(TAG,"current rom address sensor %016" PRIX64, rom);
    }
#else 
    set_gpio(ONE_WIRE_BUS);
    xTaskCreate(sampleTask, "SampleTask", 4096, NULL, 10, NULL);
#endif
}