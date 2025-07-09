#include "ds18b20.h"

static const char *TAG = "MAIN";

#define SONDA_1  0x053B70D446AF8528 //F/S
#define SONDA_2  0x6F5558D446F86828
#define SONDA_3  0x942DA6D446B54928
#define SONDA_4  0xB24C96D4463F2E28
#define SONDA_5  0x0318F1D446673428
#define SONDA_6  0x2F05FAD446134328
#define SONDA_7  0x1A12BED446364B28
#define SONDA_8  0x15027CD446ECA728
#define SONDA_9  0x477716D4462FD328
#define SONDA_10 0xC96BD9D446C3BC28
#define SONDA_19 0x50713FD44642A728

#define SONDA_1A 0xB56ED3D44648D828
#define SONDA_2A 0xC36522D446883828
#define SONDA_3A 0xF26D24D446F51A28
#define SONDA_4A 0x6F1FFFD446070E28
#define SONDA_5A 0xE30AACD446FB7028
#define SONDA_6A 0x58705AD446BE6228
#define SONDA_7A 0x1D40A6D446374A28
#define SONDA_8A 0xA5572DD446CFE828
#define SONDA_9A 0x4725B0D446927728

static uint64_t sonda[] = {
	//SONDA_1A,
	//SONDA_2A,
	//SONDA_3A,
    //SONDA_4A,
    //SONDA_5A,
    //SONDA_6A,
    //SONDA_7A,
    //SONDA_8A,
    //SONDA_9A
};

static size_t sonda_size = sizeof(sonda) / sizeof(sonda[0]);
static int unic;

static void sampleTask(void *pvParameters) {
    set_gpio(5);
    unic = (sonda_size == 0) ? 1 : 0;
    while (1){
        int16_t temp[sonda_size + unic];
        if( get_temperature (sonda, sonda_size, temp) != ESP_OK){
            ESP_LOGW(TAG,"no DS18B20 detected");
        }
        for (uint8_t i = 0; i<(sonda_size + unic); i++) {
            ESP_LOGI(TAG,"current sensor %d temperature = %.1f\n", i + 1, (float)(temp[i])/10.0f);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    xTaskCreate(sampleTask, "SampleTask", 4096, NULL, 10, NULL);

}