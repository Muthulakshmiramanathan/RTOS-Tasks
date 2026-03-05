#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "esp_log.h"

/* ---------------- GPIO DEFINITIONS ---------------- */
#define LED_GPIO          2
#define SWITCH_GPIO       4

/* ---------------- I2C DEFINITIONS ---------------- */
#define I2C_SDA           21
#define I2C_SCL           22
#define I2C_PORT          I2C_NUM_0
#define MCP9808_ADDR      0x18

/* ---------------- UART DEFINITIONS ---------------- */
#define UART_PORT         UART_NUM_0
#define BUF_SIZE          256

#define TEMP_THRESHOLD    25.0

static const char *TAG = "RTOS_SYSTEM";

/* ---------------- GLOBAL VARIABLES ---------------- */
QueueHandle_t temp_queue;
SemaphoreHandle_t switch_semaphore;

float latest_temperature = 0;
volatile uint32_t temperature_count = 0;

/* ---------------- I2C INIT ---------------- */
void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

/* ---------------- MCP9808 READ FUNCTION ---------------- */
float mcp9808_read_temp()
{
    uint8_t reg = 0x05;
    uint8_t data[2];

    i2c_master_write_read_device(I2C_PORT,
                                 MCP9808_ADDR,
                                 &reg, 1,
                                 data, 2,
                                 pdMS_TO_TICKS(1000));

    uint16_t raw = ((data[0] << 8) | data[1]) & 0x0FFF;
    return raw * 0.0625;
}

/* ---------------- TEMPERATURE TASK ---------------- */
void temperature_task(void *pv)
{
    float temp;

    while (1)
    {
        temp = mcp9808_read_temp();

        latest_temperature = temp;
        temperature_count++;

        xQueueSend(temp_queue, &temp, portMAX_DELAY);

        ESP_LOGI(TAG, "Temperature: %.2f C", temp);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ---------------- LED TASK ---------------- */
void led_task(void *pv)
{
    float temp;
    int delay_time = 1000;

    while (1)
    {
        if (xQueueReceive(temp_queue, &temp, 0))
        {
            if (temp > TEMP_THRESHOLD)
                delay_time = 200;
            else
                delay_time = 1000;
        }

        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_time));

        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_time));
    }
}

/* ---------------- SWITCH ISR ---------------- */
static void IRAM_ATTR switch_isr(void *arg)
{
    BaseType_t hp_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(switch_semaphore, &hp_task_woken);
    portYIELD_FROM_ISR(hp_task_woken);
}

/* ---------------- SWITCH TASK ---------------- */
void switch_task(void *pv)
{
    while (1)
    {
        if (xSemaphoreTake(switch_semaphore, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Switch Pressed (Interrupt Triggered)");
        }
    }
}

/* ---------------- UART TASK ---------------- */
void uart_task(void *pv)
{
    uint8_t data[BUF_SIZE];

    while (1)
    {
        int len = uart_read_bytes(UART_PORT,
                                  data,
                                  BUF_SIZE - 1,
                                  pdMS_TO_TICKS(100));

        if (len > 0)
        {
            data[len] = '\0';

            if (strstr((char *)data, "status"))
            {
                printf("\n------ SYSTEM STATUS ------\n");
                printf("Current Temperature : %.2f C\n", latest_temperature);
                printf("Total Readings      : %lu\n", temperature_count);
                printf("----------------------------\n");
            }
            else
            {
                printf("Unknown command\n");
            }
        }
    }
}

/* ---------------- MAIN FUNCTION ---------------- */
void app_main()
{
    /* GPIO Setup */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(SWITCH_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(SWITCH_GPIO, GPIO_INTR_NEGEDGE);

    /* UART Setup */
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);

    /* I2C Setup */
    i2c_init();

    /* RTOS Objects */
    temp_queue = xQueueCreate(5, sizeof(float));
    switch_semaphore = xSemaphoreCreateBinary();

    /* ISR Setup */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(SWITCH_GPIO, switch_isr, NULL);

    /* Task Creation (Priority Order) */
    xTaskCreate(switch_task, "Switch_Task", 2048, NULL, 6, NULL);
    xTaskCreate(temperature_task, "Temp_Task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_task, "UART_Task", 4096, NULL, 4, NULL);
    xTaskCreate(led_task, "LED_Task", 2048, NULL, 3, NULL);
}
