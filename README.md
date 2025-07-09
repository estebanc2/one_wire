# Minimal DS18B20 temperature sensors connected to one one-wire bus

This project brings 3 functions to operate a bus with one to N [DS18B20](assets/DS18B20.pdf) sensors connected to one gpio pin of an Espressif microcontroler.
The sensors should be connected with [3 wires](assets/graficos.pdf): pwr, gnd and data, i.e. not in parasite configuration.
Tested microcontroler were [ESP32-C3](assets/graficos.pdf) and [ESP8266](assets/graficos.pdf).
The first function, `set_gpio` is used once to set a particular gpio as the one-wire bus.
The second one, `get_temperature` returns an array of N int16_t for the temperature in °C x 10 of each sensor. This function needs a pointer and size of an array of ds18b20's rom address and after around 800ms get the result.
To know the ds18b20's rom address to full the above array there are a thirs function, get_rom() that need to receive a gpio where only one sensor should be connected and returns this uint64_t address.
So the suggested operation is first discover all rom address, running the 3er function for each sensor. After that define the array that needs the 2d function to get the temperatures. If only one sensor will be connected it is not necessary to discover it rom address. An empty rom address works as an array with one rom address.

```C
/**
 * @brief set gpio for the 1 wire bus
 * @param[in] gpio number
 * @return
 *  - ESP_OK
 *  - ESP_ERR_INVALID_ARG
 */
esp_err_t set_gpio (uint8_t);
```

```C
/**
 * @brief get temperature in °C x 10 for all ds18b20 devices in the bus
 * @param[in] array of N ds18b20 rom addresses
 * @param[out] array of N temperatures in °C x 10. -800 means not sensor response and -900 means a CRC check fails.
 * @return
 *  - ESP_OK
 *  - ESP_ERR_NOT_FOUND
 */
esp_err_t get_temperature (const uint64_t *, size_t, int16_t *);
```

```C
/**
 * @brief discover rom address
 * @param[in] gpio number
 * @param[out] ds18b20 rom addresses
 * @return
 *  - ESP_OK
 *  - ESP_ERR_INVALID_ARG
 *  - ESP_ERR_NOT_FOUND
*/
esp_err_t get_rom(uint8_t, const uint64_t *);
```
