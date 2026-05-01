#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "TOF"

#define I2C_MASTER_SCL_IO    22
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   400000

#define TOF_ADDR 0x41

#define TMF_CMD_REG        0x08
#define TMF_STATUS_REG     0x0E
#define TMF_RESULT_REG     0x20
#define TMF_START_CMD      0x01
#define TMF_READY_MASK     0x01

#define FRAME_SIZE 128  

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

esp_err_t i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, dev, buf, 2, pdMS_TO_TICKS(1000));
}

esp_err_t i2c_read_reg(uint8_t dev, uint8_t reg, uint8_t *data)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev, &reg, 1, data, 1, pdMS_TO_TICKS(1000));
}

esp_err_t i2c_read_bytes(uint8_t dev, uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev, &reg, 1, buf, len, pdMS_TO_TICKS(1000));
}


void tof_init(void)
{
    ESP_LOGI(TAG, "TMF8829 init (basic)");
}

void tof_start(void)
{
    ESP_LOGI(TAG, "Start measurement");
    i2c_write_reg(TOF_ADDR, TMF_CMD_REG, TMF_START_CMD);
}

int tof_wait_ready(void)
{
    uint8_t status = 0;

    for (int i = 0; i < 100; i++)
    {
        i2c_read_reg(TOF_ADDR, TMF_STATUS_REG, &status);

        if (status & TMF_READY_MASK)
            return 1;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return 0;
}

void tof_read_frame(uint8_t *buffer)
{
    if (i2c_read_bytes(TOF_ADDR, TMF_RESULT_REG, buffer, FRAME_SIZE) == ESP_OK)
    {
        printf("Frame: ");
        for (int i = 0; i < 16; i++)
        {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
    else
    {
        ESP_LOGE(TAG, "Read failed");
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized");

    tof_init();

    uint8_t frame[FRAME_SIZE];

    while (1)
    {
        tof_start();

        if (tof_wait_ready())
        {
            tof_read_frame(frame);
        }
        else
        {
            ESP_LOGW(TAG, "Timeout waiting for data");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}