#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_MOSI 23
#define PIN_NUM_MISO 19
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

void app_main()
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,  
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    spi_device_handle_t spi;

    // Initialize SPI bus
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // Add SPI device
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    uint8_t tx_data = 0x55;
    uint8_t rx_data = 0;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = 8;              // 8 bits
    t.tx_buffer = &tx_data;
    t.rx_buffer = &rx_data;

    while (1)
    {
        spi_device_transmit(spi, &t);

        printf("Sent: 0x%X  Received: 0x%X\n", tx_data, rx_data);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}