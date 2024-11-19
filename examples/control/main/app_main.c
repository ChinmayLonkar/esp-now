/* Control Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "esp_pm.h"

#include "driver/gpio.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
#include "esp_mac.h"
#endif

#include "espnow.h"
#include "espnow_ctrl.h"
#include "espnow_utils.h"

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/rmt.h"
#endif

#include "iot_button.h"

static const char *TAG = "app_main";

static QueueHandle_t g_button_queue = NULL;

void power_save_set(bool enable_light_sleep)
{
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 0)
#if CONFIG_IDF_TARGET_ESP32
    esp_pm_config_esp32_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S2
    esp_pm_config_esp32s2_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C3
    esp_pm_config_esp32c3_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S3
    esp_pm_config_esp32s3_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C2
    esp_pm_config_esp32c2_t pm_config = {
#endif
#else // ESP_IDF_VERSION
    esp_pm_config_t pm_config = {
#endif
            .max_freq_mhz = 160,
            .min_freq_mhz = 40,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = enable_light_sleep
#endif
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
}

static char *bind_error_to_string(espnow_ctrl_bind_error_t bind_error)
{
    switch (bind_error) {
    case ESPNOW_BIND_ERROR_NONE: {
        return "No error";
        break;
    }

    case ESPNOW_BIND_ERROR_TIMEOUT: {
        return "bind timeout";
        break;
    }

    case ESPNOW_BIND_ERROR_RSSI: {
        return "bind packet RSSI below expected threshold";
        break;
    }

    case ESPNOW_BIND_ERROR_LIST_FULL: {
        return "bindlist is full";
        break;
    }

    default: {
        return "unknown error";
        break;
    }
    }
}

static void app_wifi_init()
{
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void app_initiator_send_press_cb_1(void *arg, void *usr_data)
{
    static bool status = 0;

    ESP_ERROR_CHECK(!(BUTTON_SINGLE_CLICK == iot_button_get_event(arg)));

    ESP_LOGI(TAG, "initiator send press_1");
    button_event_t btn_evt = iot_button_get_event(arg);
    xQueueSend(g_button_queue, &btn_evt, pdMS_TO_TICKS(300));
}

static void app_initiator_send_press_cb_2(void *arg, void *usr_data)
{
    static bool status = 0;

    ESP_ERROR_CHECK(!(BUTTON_SINGLE_CLICK == iot_button_get_event(arg)));

    ESP_LOGI(TAG, "initiator send press_2");
    button_event_t btn_evt = iot_button_get_event(arg);
    btn_evt += 100;
    xQueueSend(g_button_queue, &btn_evt, pdMS_TO_TICKS(300));
}

static void app_initiator_send_press_cb_3(void *arg, void *usr_data)
{
    static uint16_t status = 33;

    ESP_ERROR_CHECK(!(BUTTON_SINGLE_CLICK == iot_button_get_event(arg)));
    ESP_LOGI(TAG, "initiator send press_3");
    button_event_t btn_evt = iot_button_get_event(arg);
    btn_evt += 200;
    xQueueSend(g_button_queue, &btn_evt, pdMS_TO_TICKS(300));
}

static void app_initiator_send_press_cb_4(void *arg, void *usr_data)
{
    static uint16_t status = 66;
    ESP_ERROR_CHECK(!(BUTTON_SINGLE_CLICK == iot_button_get_event(arg)));

    ESP_LOGI(TAG, "initiator send press_4");
    button_event_t btn_evt = iot_button_get_event(arg);
    btn_evt += 300;
    xQueueSend(g_button_queue, &btn_evt, pdMS_TO_TICKS(300));
}

static void app_initiator_bind_press_cb(void *arg, void *usr_data)
{
    ESP_ERROR_CHECK(!(BUTTON_LONG_PRESS_START == iot_button_get_event(arg)));

    ESP_LOGI(TAG, "initiator bind press");
    button_event_t btn_evt = iot_button_get_event(arg);
    xQueueSend(g_button_queue, &btn_evt, pdMS_TO_TICKS(300));
}

static void app_initiator_unbind_press_cb(void *arg, void *usr_data)
{
    ESP_ERROR_CHECK(!(BUTTON_LONG_PRESS_START == iot_button_get_event(arg)));

    ESP_LOGI(TAG, "initiator unbind press");
    button_event_t btn_evt = iot_button_get_event(arg);
    btn_evt += 100;
    xQueueSend(g_button_queue, &btn_evt, pdMS_TO_TICKS(300));
}

static void app_driver_init(void)
{

    button_config_t button_config_1 = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = 35,
            .active_level = 1,
#if CONFIG_GPIO_BUTTON_SUPPORT_POWER_SAVE
            .enable_power_save = true,
#endif
        },
    };

    button_config_t button_config_2 = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = 32, 
            .active_level = 1,
#if CONFIG_GPIO_BUTTON_SUPPORT_POWER_SAVE
            .enable_power_save = true,
#endif
        },
    };

    button_config_t button_config_3 = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = 39,
            .active_level = 1,
#if CONFIG_GPIO_BUTTON_SUPPORT_POWER_SAVE
            .enable_power_save = true,
#endif
        },
    };

    button_config_t button_config_4 = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = 34,
            .active_level = 1,
#if CONFIG_GPIO_BUTTON_SUPPORT_POWER_SAVE
            .enable_power_save = true,
#endif
        },
    };

    button_handle_t button_handle_1 = iot_button_create(&button_config_1);

    iot_button_register_cb(button_handle_1, BUTTON_SINGLE_CLICK, app_initiator_send_press_cb_1, NULL);
    iot_button_register_cb(button_handle_1, BUTTON_LONG_PRESS_START, app_initiator_bind_press_cb, NULL);

    button_handle_t button_handle_2 = iot_button_create(&button_config_2);

    iot_button_register_cb(button_handle_2, BUTTON_SINGLE_CLICK, app_initiator_send_press_cb_2, NULL);
    iot_button_register_cb(button_handle_2, BUTTON_LONG_PRESS_START, app_initiator_unbind_press_cb, NULL);

    button_handle_t button_handle_3 = iot_button_create(&button_config_3);

    iot_button_register_cb(button_handle_3, BUTTON_SINGLE_CLICK, app_initiator_send_press_cb_3, NULL);

    button_handle_t button_handle_4 = iot_button_create(&button_config_4);
    iot_button_register_cb(button_handle_4, BUTTON_SINGLE_CLICK, app_initiator_send_press_cb_4, NULL);

    gpio_config_t led_gpio_config;
    led_gpio_config.intr_type = GPIO_INTR_DISABLE;
    led_gpio_config.mode = GPIO_MODE_OUTPUT;
    led_gpio_config.pin_bit_mask = (1ULL << 26);
    led_gpio_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    led_gpio_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&led_gpio_config);

    led_gpio_config.intr_type = GPIO_INTR_DISABLE;
    led_gpio_config.mode = GPIO_MODE_OUTPUT;
    led_gpio_config.pin_bit_mask = (1ULL << 25);
    led_gpio_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    led_gpio_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&led_gpio_config);

    led_gpio_config.intr_type = GPIO_INTR_DISABLE;
    led_gpio_config.mode = GPIO_MODE_OUTPUT;
    led_gpio_config.pin_bit_mask = (1ULL << 14);
    led_gpio_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    led_gpio_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&led_gpio_config);

    gpio_set_level(26, 0);
    gpio_set_level(25, 0);
    gpio_set_level(14, 0);
}

static void control_task(void *pvParameter)
{
    button_event_t evt_data;
    uint8_t status = 0;
    
    // Create a single queue for all buttons
    g_button_queue = xQueueCreate(20, sizeof(button_event_t));
    if (!g_button_queue) {
        ESP_LOGE(TAG, "Error creating button queue.");
        return;
    }

#if CONFIG_PM_ENABLE
    gpio_wakeup_enable(34, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(35, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(32, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(33, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
#endif

    while(1) {
        if (xQueueReceive(g_button_queue, &evt_data, pdMS_TO_TICKS(10)) == pdTRUE) {
            power_save_set(false);
            esp_wifi_force_wakeup_acquire();
            
            // Handle button events based on which callback sent them
            if (evt_data == BUTTON_SINGLE_CLICK) {
                gpio_set_level(14, 1);
                vTaskDelay(200/portTICK_PERIOD_MS);
                gpio_set_level(14, 0);
                // Button 1 single click
                espnow_ctrl_initiator_send(ESPNOW_ATTRIBUTE_KEY_1, ESPNOW_ATTRIBUTE_WINDOW_COVERING_UP, true);
                ESP_LOGI(TAG, "Button 1 single click");
            } else if (evt_data == BUTTON_LONG_PRESS_START) {
                gpio_set_level(26, 1);
                vTaskDelay(300/portTICK_PERIOD_MS);
                gpio_set_level(26, 0);
                vTaskDelay(300/portTICK_PERIOD_MS);
                gpio_set_level(26, 1);
                vTaskDelay(300/portTICK_PERIOD_MS);
                gpio_set_level(26, 0);
                // Button 1 long press - bind all
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_1, true);
                vTaskDelay(300/portTICK_PERIOD_MS);
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_2, true);
                vTaskDelay(300/portTICK_PERIOD_MS);
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_3, true);
                vTaskDelay(300/portTICK_PERIOD_MS);
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_4, true);
                ESP_LOGI(TAG, "Button 1 long press - bind all");
            } else if (evt_data == BUTTON_SINGLE_CLICK + 100) {
                gpio_set_level(14, 1);
                vTaskDelay(200/portTICK_PERIOD_MS);
                gpio_set_level(14, 0);
                // Button 2 single click
                espnow_ctrl_initiator_send(ESPNOW_ATTRIBUTE_KEY_2, ESPNOW_ATTRIBUTE_WINDOW_COVERING_DOWN, false);
                ESP_LOGI(TAG, "Button 2 single click");
            } else if (evt_data == BUTTON_LONG_PRESS_START + 100) {
                gpio_set_level(26, 1);
                vTaskDelay(300/portTICK_PERIOD_MS);
                gpio_set_level(26, 0);
                vTaskDelay(300/portTICK_PERIOD_MS);
                gpio_set_level(26, 1);
                vTaskDelay(300/portTICK_PERIOD_MS);
                gpio_set_level(26, 0);
                // Button 2 long press - unbind all
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_1, false);
                vTaskDelay(300/portTICK_PERIOD_MS);
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_2, false);
                vTaskDelay(300/portTICK_PERIOD_MS);
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_3, false);
                vTaskDelay(300/portTICK_PERIOD_MS);
                espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_4, false);
                ESP_LOGI(TAG, "Button 2 long press - unbind all");
            } else if (evt_data == BUTTON_SINGLE_CLICK + 200) {
                gpio_set_level(14, 1);
                vTaskDelay(200/portTICK_PERIOD_MS);
                gpio_set_level(14, 0);
                // Button 3 single click
                espnow_ctrl_initiator_send(ESPNOW_ATTRIBUTE_KEY_3, ESPNOW_ATTRIBUTE_WINDOW_COVERING_STOP, 33);
                ESP_LOGI(TAG, "Button 3 single click");
            } else if (evt_data == BUTTON_SINGLE_CLICK + 300) {
                gpio_set_level(14, 1);
                vTaskDelay(200/portTICK_PERIOD_MS);
                gpio_set_level(14, 0);
                // Button 4 single click
                espnow_ctrl_initiator_send(ESPNOW_ATTRIBUTE_KEY_4, ESPNOW_ATTRIBUTE_WINDOW_COVERING_POSITION, 66);
                ESP_LOGI(TAG, "Button 4 single click");
            }

            esp_wifi_force_wakeup_release();
            power_save_set(true);
        }
    }

    // Cleanup
    if (g_button_queue) {
        vQueueDelete(g_button_queue);
        g_button_queue = NULL;
    }
    vTaskDelete(NULL);
}

static void app_responder_ctrl_data_cb(espnow_attribute_t initiator_attribute,
                              espnow_attribute_t responder_attribute,
                              uint32_t status)
{
    ESP_LOGI(TAG, "app_responder_ctrl_data_cb, initiator_attribute: %d, responder_attribute: %d, value: %" PRIu32 "",
             initiator_attribute, responder_attribute, status);
}

static void app_responder_init(void)
{
    ESP_ERROR_CHECK(espnow_ctrl_responder_bind(30 * 1000, -55, NULL));
    espnow_ctrl_responder_data(app_responder_ctrl_data_cb);
}

void app_main(void)
{
    espnow_storage_init();

    app_wifi_init();
    app_driver_init();
    power_save_set(true);
    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
    espnow_config.send_max_timeout = portMAX_DELAY;
    espnow_init(&espnow_config);
    esp_now_set_wake_window(0);
    xTaskCreate(control_task, "control_task", 4096*2, NULL, 15, NULL);
}
