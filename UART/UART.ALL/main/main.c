#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_system.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 128

QueueHandle_t number_queue;

/* ---------------- TASK 1 ---------------- */
/* Read decimal from USB UART0 */
void input_task(void *pvParameters)
{
    uint8_t data[BUF_SIZE];

    while (1)
    {
        int len = uart_read_bytes(UART_PORT,
                                  data,
                                  BUF_SIZE - 1,
                                  pdMS_TO_TICKS(1000));

        if (len > 0)
        {
            data[len] = '\0';

            int decimal_value = atoi((char *)data);

            printf("Task1: Decimal Received = %d\n", decimal_value);

            xQueueSend(number_queue, &decimal_value, portMAX_DELAY);
        }
    }
}

/* ---------------- TASK 2 ---------------- */
/* Convert received number to HEX */
void convert_task(void *pvParameters)
{
    int received_number;
    char tx_buffer[50];

    while (1)
    {
        if (xQueueReceive(number_queue,
                          &received_number,
                          portMAX_DELAY))
        {
            sprintf(tx_buffer,
                    "Task2: HEX Value = 0x%X\r\n",
                    received_number);

            uart_write_bytes(UART_PORT,
                             tx_buffer,
                             strlen(tx_buffer));
        }
    }
}

/* ---------------- MAIN ---------------- */
void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);

    number_queue = xQueueCreate(5, sizeof(int));

    if (number_queue == NULL)
    {
        printf("Queue creation failed!\n");
        return;
    }

    xTaskCreate(input_task, "Input_Task", 2048, NULL, 5, NULL);
    xTaskCreate(convert_task, "Convert_Task", 2048, NULL, 5, NULL);

    printf("System Ready. Enter Decimal Number:\n");
}