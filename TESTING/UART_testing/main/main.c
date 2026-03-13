#include <stdio.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

void app_main()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_PORT, &uart_config);

    uart_driver_install(UART_PORT, BUF_SIZE, 0, 0, NULL, 0);

    uint8_t data[BUF_SIZE];

    printf("Type something and press ENTER\n");

    while(1)
    {
        int len = uart_read_bytes(UART_PORT,
                                  data,
                                  BUF_SIZE,
                                  1000 / portTICK_PERIOD_MS);

        if(len > 0)
        {
            data[len] = '\0';
            printf("Received: %s\n", data);
        }
    }
}