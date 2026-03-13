#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SDA 21
#define SCL 22
#define I2C_PORT I2C_NUM_0
#define MCP9808_ADDR 0x18
#define TEMP_REG 0x05

void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA,
        .scl_io_num = SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

float read_temperature()
{
    uint8_t data[2];

    i2c_master_write_read_device(
        I2C_PORT,
        MCP9808_ADDR,
        (uint8_t*)&(uint8_t){TEMP_REG},
        1,
        data,
        2,
        1000 / portTICK_PERIOD_MS
    );

    int raw = ((data[0] & 0x1F) << 8) | data[1];
    float temp = raw * 0.0625;

    return temp;
}

void app_main()
{
    i2c_init();

    float temp[5];

    while(1)
    {
        for(int i=0;i<5;i++)
        {
            temp[i] = read_temperature();
            printf("Temp %d = %.2f C\n", i+1, temp[i]);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        float sum = 0;

        for(int i=0;i<5;i++)
        {
            sum += temp[i];
        }

        printf("Total Sum = %.2f\n", sum);

        float verify = temp[0] + temp[1] + temp[2] + temp[3] + temp[4];

        if(sum == verify)
            printf("Verification PASS\n");
        else
            printf("Verification FAIL\n");

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
