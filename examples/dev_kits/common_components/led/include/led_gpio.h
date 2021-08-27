// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "sdkconfig.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_GPIO_DARK_LOW = 0,       /**< choose this parameter if the led is dark when the level of gpio is low */
    LED_GPIO_DARK_HIGH = 1,      /**< choose this parameter if the led is dark when the level of gpio is high */
} led_dark_level_t;

typedef enum {
    LED_GPIO_OFF,
    LED_GPIO_ON,
    LED_GPIO_QUICK_BLINK,
    LED_GPIO_SLOW_BLINK,
} led_gpio_status_t;

typedef enum {
    LED_GPIO_NORMAL_MODE,        /**< led normal */
    LED_GPIO_NIGHT_MODE,         /**< led night mode, always turn off led */
} led_gpio_mode_t;

typedef void *led_handle_t;

/**
  * @brief  led initialize.
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t led_gpio_setup();

/**
 * @brief led blink frequency update
 * @param quick_blink_freq quick blink frequency
 * @param slow_blink_freq slow blink frequency
 * @return
 *     - ESP_OK: success
 *     - others: fail
 */
esp_err_t led_gpio_update_blink_freq(int quick_blink_freq, int slow_blink_freq);

/**
  * @brief  create new led.
  *
  * @param  io_num
  * @param  dark_level on whick level the led is dark
  *
  * @return the handle of the led
  */
led_handle_t led_gpio_create(uint8_t io_num, led_dark_level_t dark_level);

/**
  * @brief  free the memory of led
  *
  * @param  led_handle
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t led_gpio_delete(led_handle_t led_handle);

/**
  * @brief  set state of led
  *
  * @param  led_handle
  * @param  state refer to enum led_gpio_status_t
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t led_gpio_state_write(led_handle_t led_handle, led_gpio_status_t state);

/**
  * @brief  set mode of led
  *
  * @param  led_handle
  * @param  mode refer to enum led_gpio_mode_t
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t led_gpio_mode_write(led_handle_t led_handle, led_gpio_mode_t mode);

/**
  * @brief  set duty of night mode, all the leds share the same duty
  *
  * @param  duty value from 0~100
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t led_gpio_night_duty_write(uint8_t duty);

/**
  * @brief  get state of led
  *
  * @param  led_handle
  *
  * @return state of led
  */
led_gpio_status_t led_gpio_state_read(led_handle_t led_handle);

/**
  * @brief  get mode of led
  *
  * @param  led_handle
  *
  * @return mode of led
  */
led_gpio_mode_t led_gpio_mode_read(led_handle_t led_handle);

/**
  * @brief  get duty of night mode
  *
  * @return duty of night mode
  */
uint8_t led_gpio_night_duty_read();

#ifdef __cplusplus
}
#endif