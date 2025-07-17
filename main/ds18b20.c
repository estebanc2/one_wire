#include "ds18b20.h"

// ROM commands
#define MATCH_ROM 0x55
#define SKIP_ROM 0xCC
#define READ_ROM 0x33
//Function commands
#define CONVERT_T 0x44
#define READ_SCRATCH 0xBE

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
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    if (gpio_config(&config) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}
/*
Reset Pulse:

          | RESET_PULSE | RESET_WAIT_DURATION |
          | _DURATION   |                     |
          |             |   | | RESET     |   |
          |             | * | | _PRESENCE |   |
          |             |   | | _DURATION |   |
----------+             +-----+           +--------------
          |             |     |           |
          |             |     |           |
          |             |     |           |
          +-------------+     +-----------+
*: RESET_PRESENCE_WAIT_DURATION
*/

#define MASTER_RESET_PULSE 490 // Reset time high. Reset time low.
#define DS18B20_MAX_WAITS 60    // Presence detect high.
#define DS18B20_MAX_TX_PRESENCE 240 // Presence detect low.

static esp_err_t initialize (){
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    esp_delay_us(MASTER_RESET_PULSE);
    gpio_set_level(pin, 1);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    //esp_delay_us(RECOVERY_DURATION);
    uint8_t response_time = 0;
    while (gpio_get_level(pin) == 1)
    {
        if (response_time > DS18B20_MAX_WAITS)
        {
            return ESP_ERR_TIMEOUT;
        }
        ++response_time;
        esp_delay_us(1);
    }
    uint8_t presence_time = 0;
    while (gpio_get_level(pin) == 0)
    {
        if (presence_time > DS18B20_MAX_TX_PRESENCE)
        {
            return ESP_ERR_TIMEOUT;
        }
        ++presence_time;
        esp_delay_us(1);
    }
    esp_delay_us(MASTER_RESET_PULSE - response_time - presence_time);
    return ESP_OK;
}
/*
Write 1 bit:

          | SLOT_START | SLOT_BIT  | SLOT_RECOVERY | NEXT
          | _DURATION  | _DURATION | _DURATION     | SLOT
          |            |           |               |
----------+            +-------------------------------------
          |            |
          |            |
          |            |
          +------------+

Write 0 bit:

          | SLOT_START | SLOT_BIT  | SLOT_RECOVERY | NEXT
          | _DURATION  | _DURATION | _DURATION     | SLOT
          |            |           |               |
----------+                        +-------------------------
          |                        |
          |                        |
          |                        |
          +------------------------+

Read 1 bit:


          | SLOT_START | SLOT_BIT_DURATION | SLOT_RECOVERY | NEXT
          | _DURATION  |                   | _DURATION     | SLOT
          |            | SLOT_BIT_   |     |               |
          |            | SAMPLE_TIME |     |               |
----------+            +----------------------------------------------
          |            |
          |            |
          |            |
          +------------+

Read 0 bit:

          | SLOT_START | SLOT_BIT_DURATION | SLOT_RECOVERY | NEXT
          | _DURATION  |                   | _DURATION     | SLOT
          |            | SLOT_BIT_   |     |               |
          |            | SAMPLE_TIME |     |               |
----------+            |             |  +-----------------------------
          |            |                |
          |            |   PULLED DOWN  |
          |            |    BY DEVICE   |
          +-----------------------------+          
*/

#define SLOT_START_DURATION      2  // bit start pulse duration
#define SLOT_BIT_DURATION       60  // duration for each bit to transmit
#define SLOT_RECOVERY_DURATION   5  // recovery time between each bit, should be longer in parasite power mode
#define SLOT_BIT_SAMPLE_TIME    15  

static void write_1_bit(const uint8_t bit)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    if (bit == 0)
    {
        esp_delay_us(SLOT_START_DURATION + SLOT_BIT_DURATION);//MASTER_WRITE_0);
        gpio_set_level(pin, 1);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
    } else
    {
        esp_delay_us(SLOT_START_DURATION);//MASTER_WRITE_1);
        gpio_set_level(pin, 1);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        esp_delay_us(SLOT_BIT_DURATION);//MASTER_WRITE_0 - MASTER_WRITE_1);
    }
    esp_delay_us(SLOT_RECOVERY_DURATION);//RECOVERY_DURATION);
}

static uint8_t read_1_bit(void)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    esp_delay_us(SLOT_START_DURATION);//MASTER_READ_INIT);
    gpio_set_level(pin, 1);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    esp_delay_us(SLOT_BIT_SAMPLE_TIME);//MASTER_READ - MASTER_READ_INIT);
    uint8_t bit = gpio_get_level(pin);
    esp_delay_us(SLOT_BIT_DURATION - SLOT_BIT_SAMPLE_TIME + SLOT_RECOVERY_DURATION);//READ_TIME_SLOT - MASTER_READ_INIT - MASTER_READ);
    return bit;
}

static void send_1_byte (uint8_t byte){
    for (uint8_t i = 0; i < 8; ++i){
        write_1_bit(byte & 1);
        byte >>= 1;
    }
}

static uint8_t read_1_byte (void){
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        byte >>= 1;
        if (read_1_bit() != 0)
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

static esp_err_t crc_check (uint8_t *data, size_t len){
    uint8_t calculated_crc = compute_crc8((const uint8_t *)data, len-1);
    if (calculated_crc == data[len-1]){
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_CRC;
    }
}

esp_err_t get_temperature(const uint64_t *sonda, size_t sondas, int16_t *temp){
    if (initialize() != ESP_OK){
        return ESP_ERR_NOT_FOUND;
    }
    send_1_byte(SKIP_ROM);
    send_1_byte(CONVERT_T);
    vTaskDelay(750 / portTICK_PERIOD_MS);
    for (size_t i = 0; i < sondas; i++) {
        if (initialize() != ESP_OK){
            if (temp) temp[i] = -8080; //ESP_ERR_NOT_FOUND; 
        } else {
            if (sonda[0] != 0) { 
                send_1_byte(MATCH_ROM);
                uint64_t pos = 0;
                for (uint8_t k = 0; k < 64; k++){
                     pos = (uint64_t)1 << k;
                    if ((sonda[i] & pos) == 0){
                        write_1_bit(0);
                    } else {
                        write_1_bit(1);
                        }
                }
            } else {
                send_1_byte(SKIP_ROM);
            }
            send_1_byte(READ_SCRATCH);
            rx_t rx = {0};
            for (uint8_t j = 0; j < 9; ++j)
            {
                rx.b[j] = read_1_byte();
            }
            if (crc_check(rx.b, 9) != ESP_OK){
                if (temp) temp[i] = -9000;//ESP_ERR_INVALID_CRC
            } else{
                if (temp) temp[i] = (100 * (int16_t)((rx.b[1] << 8) | rx.b[0]))/ 16;
            }
        }
    }
    return ESP_OK;
}

esp_err_t get_rom(uint8_t pin, uint64_t * rom){
    if (set_gpio(pin) != ESP_OK){
        return ESP_ERR_INVALID_ARG;
    }
    if (initialize() != ESP_OK){
        return ESP_ERR_NOT_FOUND; 
    }
    send_1_byte(READ_ROM);
    uint8_t rx[8] = {0};
    for (uint8_t i = 0; i < 8; i++){
        rx[i] = read_1_byte();
    }
    if (crc_check(rx, 8) != ESP_OK){
        return ESP_ERR_INVALID_CRC; 
    }
    uint64_t code = 0;
    for (int i = 0; i < 8; i++) {
        code |= ((uint64_t)rx[i]) << (8 * i);
    }
    if (rom) {
        *((uint64_t*)rom) = code;
    }
    return ESP_OK;
}