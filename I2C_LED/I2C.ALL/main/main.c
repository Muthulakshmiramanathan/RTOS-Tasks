#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

/* ---------------- I2C CONFIG ---------------- */
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

/* ---------------- MCP9808 CONFIG ---------------- */
#define MCP9808_ADDR 0x18
#define TEMP_REG 0x05

/* ---------------- LED CONFIG ---------------- */
#define LED_PIN GPIO_NUM_2

/* ---------------- QUEUE HANDLE ---------------- */
QueueHandle_t temp_queue;

/* ---------------- I2C INITIALIZATION ---------------- */
void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/* ---------------- READ TEMPERATURE FUNCTION ---------------- */
float read_temperature()
{
    uint8_t reg = TEMP_REG;
    uint8_t data[2];

    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MCP9808_ADDR,
        &reg, 1,
        data, 2,
        1000 / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK)
    {
        printf("I2C Read Error!\n");
        return 0;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    raw &= 0x0FFF;

    return raw * 0.0625;
}

/* ---------------- I2C TASK ---------------- */
void i2c_task(void *pvParameters)
{
    float temperature;

    while (1)
    {
        temperature = read_temperature();

        xQueueSend(temp_queue, &temperature, portMAX_DELAY);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/* ---------------- LED TASK ---------------- */
void led_task(void *pvParameters)
{
    float received_temp;

    while (1)
    {
        if (xQueueReceive(temp_queue, &received_temp, portMAX_DELAY))
        {
            if (received_temp > 26.90)
            {
                gpio_set_level(LED_PIN, 1);
            }
            else
            {
                gpio_set_level(LED_PIN, 0);
            }
        }
    }
}

/* ---------------- DISPLAY TASK ---------------- */
void display_task(void *pvParameters)
{
    float received_temp;

    while (1)
    {
        if (xQueueReceive(temp_queue, &received_temp, portMAX_DELAY))
        {
            printf("Temperature: %.2f °C\n", received_temp);
        }
    }
}

/* ---------------- MAIN FUNCTION ---------------- */
void app_main(void)
{
    /* Initialize I2C */
    i2c_master_init();

    /* Configure LED as Output */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    /* Create Queue */
    temp_queue = xQueueCreate(5, sizeof(float));

    /* Create Tasks */
    xTaskCreate(i2c_task, "I2C_Task", 2048, NULL, 5, NULL);
    xTaskCreate(led_task, "LED_Task", 2048, NULL, 5, NULL);
    xTaskCreate(display_task, "Display_Task", 2048, NULL, 5, NULL);
}
