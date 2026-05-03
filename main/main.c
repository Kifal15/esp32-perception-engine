#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#define I2C_MASTER_SCL_IO    22
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   400000

#define TOF_ADDR 0x41

#define ENABLE_REGISTER 0xF8


#define ENABLE_PON        (1 << 2)
#define ENABLE_CPU_READY  (1 << 7)

void i2c_master_init(void)
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

esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};

    return i2c_master_write_to_device(
        I2C_MASTER_NUM,
        TOF_ADDR,
        data,
        2,
        pdMS_TO_TICKS(1000)
    );
}

esp_err_t read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_write_read_device(
        I2C_MASTER_NUM,
        TOF_ADDR,
        &reg, 1,        
        value, 1,       
        pdMS_TO_TICKS(1000)
    );
}

void app_main(void)
{
    i2c_master_init();

    if (write_reg(ENABLE_REGISTER, ENABLE_PON) != ESP_OK)
    {
        printf("I2C WRITE FAILED\n");
        return;
    }

    bool ready = false;
    uint8_t status = 0;

    for (int i = 0; i < 50; i++)
    {
        if (read_reg(ENABLE_REGISTER, &status) != ESP_OK)
        {
            printf("I2C READ FAILED\n");
            return;
        }

        if (status & ENABLE_CPU_READY)
        {
            printf("CPU READY\n");
            ready = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!ready)
    {
        printf("ERROR: CPU NOT READY\n");
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}