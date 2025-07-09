#include "ds18b20.h"

// ROM commands
#define MATCH_ROM 0x55
#define SKIP_ROM 0xCC
//Function commands
#define CONVERT_T 0x44
#define READ_SCRATCH 0xBE
//config. resolution
#define RES_12 0x7F 
//1 wire times
#define MASTER_RESET_PULSE_DURATION 480 // Reset time high. Reset time low.
#define RESPONSE_MAX_DURATION 60        // Presence detect high.
#define PRESENCE_PULSE_MAX_DURATION 240 // Presence detect low.
#define RECOVERY_DURATION 1             // Bus recovery time.
#define TIME_SLOT_START_DURATION 1      // Time slot start.
#define TIME_SLOT_DURATION 80           // Time slot.
#define VALID_DATA_DURATION 15          // Valid data duration.

#ifdef CONFIG_IDF_TARGET_ESP8266
#define esp_delay_us(x) os_delay_us(x)
#else
#define esp_delay_us(x) esp_rom_delay_us(x)
#endif

static uint8_t pin;

esp_err_t set_gpio(uint8_t gpio)
{
    pin = gpio;
    gpio_config_t config;
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = (1ULL << pin);
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    if (gpio_config(&config) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t initialize (){
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    esp_delay_us(MASTER_RESET_PULSE_DURATION);
    //gpio_set_level(pin, 1);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    esp_delay_us(RECOVERY_DURATION);
    uint8_t response_time = 0;
    while (gpio_get_level(pin) == 1)
    {
        if (response_time > RESPONSE_MAX_DURATION)
        {
            return ESP_ERR_TIMEOUT;
        }
        ++response_time;
        esp_delay_us(1);
    }
    uint8_t presence_time = 0;
    while (gpio_get_level(pin) == 0)
    {
        if (presence_time > PRESENCE_PULSE_MAX_DURATION)
        {
            return ESP_ERR_TIMEOUT;
        }
        ++presence_time;
        esp_delay_us(1);
    }
    esp_delay_us(MASTER_RESET_PULSE_DURATION - response_time - presence_time);
    return ESP_OK;
}

static uint8_t _read_bit(void)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    esp_delay_us(TIME_SLOT_START_DURATION);
    //gpio_set_level(pin, 1);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    esp_delay_us(VALID_DATA_DURATION - TIME_SLOT_START_DURATION);
    uint8_t bit = gpio_get_level(pin);
    esp_delay_us(TIME_SLOT_DURATION - RECOVERY_DURATION - VALID_DATA_DURATION);
    return bit;
}

static void _send_bit(const uint8_t bit)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    if (bit == 0)
    {
        esp_delay_us(58);//TIME_SLOT_DURATION);
        //gpio_set_level(pin, 1);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        esp_delay_us(RECOVERY_DURATION);
    }
    else
    {
        esp_delay_us(TIME_SLOT_START_DURATION);
        //gpio_set_level(pin, 1);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        esp_delay_us(TIME_SLOT_DURATION);
    }
}

static void send_1_byte (uint8_t byte){
    for (uint8_t i = 0; i < 8; ++i){
        _send_bit(byte & 1);
        byte >>= 1;
    }
}

static uint8_t read_1_byte (void){
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        byte >>= 1;
        if (_read_bit() != 0)
        {
            byte |= 0x80;
        }
    }
    return byte;
}

static uint8_t compute_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    uint8_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static esp_err_t crc_check (rx_t rx){
	rx_t rx_to_test = rx;
    // Calculamos el CRC de los primeros 7 bytes
    uint8_t calculated_crc = compute_crc8((const uint8_t *)&rx_to_test, 8);
    //printf("calc: %d, recei: %d\n", calculated_crc, rx.b[8]);
    if (calculated_crc == rx.b[8]){
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_CRC;
    }
}

esp_err_t get_temperature(const uint64_t *sonda, size_t sondas, int16_t *temp_x_10){
    rx_t rx = {0};
    int16_t temp_x_10[sondas];
    if (initialize() != ESP_OK){
        return ESP_ERR_NOT_FOUND; //there are 
    }
    send_1_byte(SKIP_ROM);
    send_1_byte(CONVERT_T);
    vTaskDelay(750 / portTICK_PERIOD_MS);
    if (initialize() != ESP_OK){
        return ESP_ERR_INVALID_STATE; 
    }
    for (size_t i = 0; i < sondas; i++) {
        send_1_byte(MATCH_ROM);
        uint64_t pos = 0;
        for (uint8_t i = 0; i <64; i++){
            pos = (uint64_t)1 << i;
            if ((sonda[i] & pos) == 0){
                _send_bit(0);
            } else {
                _send_bit(1);
            }
        }
        send_1_byte(READ_SCRATCH);
        for (uint8_t i = 0; i < 9; ++i)
        {
            rx.b[i] = read_1_byte();
        }
        if (crc_check(rx) != ESP_OK){
            temp_x_10[i] = -970;//return ESP_ERR_INVALID_CRC; 
        } else{
            temp_x_10[i] = (10 * (int16_t)((rx.b[1] << 8) | rx.b[0]))/ 16;
        }
    }
    return ESP_OK;
}

esp_err_t get_rom(uint8_t, const uint64_t *){
    return ESP_OK;
}