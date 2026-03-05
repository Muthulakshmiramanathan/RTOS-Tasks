#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define SWITCH_PIN GPIO_NUM_5
#define OUTPUT_PIN GPIO_NUM_2

void switch_task(void *pvParameters)
{
    int last_state = 1;   // Pull-up default HIGH
    int relay_state = 0;

    while (1)
    {
        int current_state = gpio_get_level(SWITCH_PIN);

        // Detect button press (HIGH → LOW)
        if (last_state == 1 && current_state == 0)
        {
            relay_state = !relay_state;  // Toggle relay
            gpio_set_level(OUTPUT_PIN, relay_state);

            if (relay_state)
                printf("Relay ON\n");
            else
                printf("Relay OFF\n");

            vTaskDelay(pdMS_TO_TICKS(300)); 
        }
       last_state = current_state; // Update last state
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }

}

void app_main(void)
{
    // Configure Switch Pin
    gpio_reset_pin(SWITCH_PIN);
    gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_PIN, GPIO_PULLUP_ONLY);

    // Configure Output Pin
    gpio_reset_pin(OUTPUT_PIN);
    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OUTPUT_PIN, 0);

    printf("Switch Control Started...\n");

    // Create RTOS Task
    xTaskCreate(
        switch_task,
        "Switch_Task",
        4096,
        NULL,
        5,
        NULL
    );
}