# Minimal DS18B20 temperature sensors connected to one one-wire bus

This project provides 3 functions to operate a bus with one to N [DS18B20](assets/DS18B20.pdf) sensors connected to a single GPIO pin of an Espressif microcontroller.

- The sensors should be connected with [3 wires](assets/graficos.pdf): power, ground, and data, **not** in parasite configuration (where the sensor is powered only through the data line).  
- Sensors remain at default configuration, 12-bit resolution. These are [analizer capture](assets/one%20wire%20timmings.pdf) to show one wire protocol behavior.
- Tested microcontrollers: [ESP32-C3](assets/graficos.pdf) and [ESP8266](assets/graficos.pdf).

## Functions

1. **set_gpio**: Used once to set a particular GPIO as the one-wire bus.
2. **get_temperature**: Returns an array of N `int16_t` values, each representing the temperature in °C x 100 for each sensor. This function requires a pointer and size of an array of DS18B20 ROM addresses. After about 850ms, it returns the result.
3. **get_rom**: Used to discover the ROM address of a sensor. This function requires a GPIO where only one sensor should be connected and returns the sensor's `uint64_t` ROM address.

## API Reference

```C
/**
 * @brief Set GPIO for the 1-wire bus
 * @param[in] gpio GPIO number
 * @return
 *  - ESP_OK
 *  - ESP_ERR_INVALID_ARG
 */
esp_err_t set_gpio(uint8_t);
```

```C
/**
 * @brief Get temperature in °C x 100 for all DS18B20 devices on the bus
 * @param[in]  array of N DS18B20 ROM addresses
 * @param[out] array of N temperatures in °C x 100. -8000 means no sensor response, -9000 means CRC check failed.
 * @return
 *  - ESP_OK
 *  - ESP_ERR_NOT_FOUND
 */
esp_err_t get_temperature(const uint64_t *, size_t, int16_t *);
```

```C
/**
 * @brief Discover ROM address
 * @param[in]  gpio GPIO number
 * @param[out] pointer to uint64_t to store the DS18B20 ROM address
 * @return
 *  - ESP_OK
 *  - ESP_ERR_INVALID_ARG
 *  - ESP_ERR_NOT_FOUND
 */
esp_err_t get_rom(uint8_t, uint64_t *);
```

## Suggested usage

First, discover all ROM addresses by running `get_rom()` for each sensor (one at a time on the bus). Then, define the array needed by `get_temperature()` to read all sensors.  
If only one sensor will be connected, it is not necessary to discover its ROM address; a 0 (zero) as ROM address works as a ROM address.

## Example usage: main.c

The main.c is an usage example with a peridic task that ask for temperature of all the sensors in the bus. Also uncommenting the DISCOVER_ROM define, the app works as a discovers rom address for each ds18b20 device connected to the assigned port.
