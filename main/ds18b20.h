
#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "stddef.h"
#include "esp_log.h"

typedef struct {
	uint8_t b[9];
} rx_t;
esp_err_t set_gpio (uint8_t);
esp_err_t get_temperature (const uint8_t *, int16_t *);