#include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include <string.h>

static const char *TAG = "example";

#define PIN_GG1 19
#define PIN_RR1 20
#define PIN_GG2 21
#define PIN_RR2 33
#define PIN_LA 34
#define PIN_LC 45
#define PIN_CLK 0
#define PIN_OE 35
#define PIN_BB1 40
#define PIN_BB2 39
#define PIN_LB 38
#define PIN_LD 37
#define PIN_LAT 36
#define DATA_LEN_BYTE 64

spi_device_handle_t hub_handle;

static inline uint8_t my_from_rgb(int r, int g, int b, int phase) {
    uint8_t result = 0;
    result |= ((r >> 3) > phase);
    result |= ((g >> 3) > phase) << 1;
    result |= ((b >> 3) > phase) << 2;
    result |= ((r >> 3) > phase) << 3;
    result |= ((g >> 3) > phase) << 4;
    result |= ((b >> 3) > phase) << 5;
    return result;
}

uint8_t data[DATA_LEN_BYTE];

void IRAM_ATTR app_main(void)
{
    spi_bus_config_t spi_config = {
        .mosi_io_num = PIN_RR1, //最低位, 0x01
        .miso_io_num = PIN_GG1,
        .quadwp_io_num = PIN_BB1,
        .quadhd_io_num = PIN_RR2,
        .data4_io_num = PIN_GG2,
        .data5_io_num = PIN_BB2, //最高位, 0x20
        .data6_io_num = 12, //暂时没用, 随便写一个没用到的IO过检查
        .data7_io_num = 13, //暂时没用, 随便写一个没用到的IO过检查
        .sclk_io_num = PIN_CLK,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_OCTAL,
        .intr_flags = 0,
    };

    spi_bus_initialize(SPI2_HOST, &spi_config, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t interface_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = 10 * 1000 * 1000,
        .input_delay_ns = 0,
        .spics_io_num = PIN_OE, //OE脚, 写入时高电平, NOTE 这里有Latch的问题
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_POSITIVE_CS,
        .queue_size = 20,
    };
    spi_bus_add_device(SPI2_HOST, &interface_config, &hub_handle);

    gpio_reset_pin(PIN_LAT);
    gpio_set_direction(PIN_LAT, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_LA);
    gpio_set_direction(PIN_LA, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_LB);
    gpio_set_direction(PIN_LB, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_LC);
    gpio_set_direction(PIN_LC, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_LD);
    gpio_set_direction(PIN_LD, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "start transaction");

    int line = 0;
    int r = 255;
    int g = 10;
    int b = 80;

    int r2 = 255;
    int g2 = 80;
    int b2 = 10;

    spi_transaction_t trans = {
        .flags = SPI_TRANS_MODE_OCT,
        .length = 8 * DATA_LEN_BYTE,
        .rxlength = 0,
        .tx_buffer = data,
        .rx_buffer = NULL,
    };

    while(1) {
        line++;
        line %= 16;

        int cur_r = (r * line) >> 4;
        int cur_g = (g * line) >> 4;
        int cur_b = (b * line) >> 4;
        int curr_r2 = (r2 * line) >> 4;
        int curr_g2 = (g2 * line) >> 4;
        int curr_b2 = (b2 * line) >> 4;

        gpio_set_level(PIN_LA, (line & 0x01) != 0);
        gpio_set_level(PIN_LB, (line & 0x02) != 0);
        gpio_set_level(PIN_LC, (line & 0x04) != 0);
        gpio_set_level(PIN_LD, (line & 0x08) != 0);
        
        for(int cnt = 0; cnt < 32; cnt++) { //5位颜色深度, 要刷新32次
            for(int i = 0; i < 64; i++) {
                int pr = ((cur_r * i) >> 6) + ((curr_r2 * (63 - i)) >> 6);
                int pg = ((cur_g * i) >> 6) + ((curr_g2 * (63 - i)) >> 6);
                int pb = ((cur_b * i) >> 6) + ((curr_b2 * (63 - i)) >> 6);
                data[i] = my_from_rgb(pr, pg, pb, cnt);
            }
            spi_device_polling_transmit(hub_handle, &trans);
            gpio_set_level(PIN_LAT, 1);
            gpio_set_level(PIN_LAT, 0);
        }
    }
}
