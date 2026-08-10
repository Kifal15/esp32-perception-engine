#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"


#define I2C_PORT_NUM 0 
#define GPIO_NUM_SDA 21
#define GPIO_NUM_SCL 22
#define I2C_FREQUENCY 100000 
#define TOF_ADDRESS 0x41 

static const char *TAG = "MAIN";



void app_main(void){
i2c_master_bus_handle_t tmf_bus ; 

i2c_master_bus_config_t tmf_bus_cfg = {

    .i2c_port   = I2C_PORT_NUM , 
    .sda_io_num = GPIO_NUM_SDA ,
    .scl_io_num = GPIO_NUM_SCL ,
    .flags.enable_internal_pullup = true,    
    .glitch_ignore_cnt = 7 , 
    .clk_source = I2C_CLK_SRC_DEFAULT ,

} ; 



i2c_new_master_bus(&tmf_bus_cfg , &tmf_bus); 


i2c_device_config_t tmf_device_cfg = { 
    
    .dev_addr_length = I2C_ADDR_BIT_LEN_7 ,
    .device_address = TOF_ADDRESS ,
    .scl_speed_hz  = I2C_FREQUENCY ,

};

i2c_master_dev_handle_t tmf_device ; 

i2c_master_bus_add_device(tmf_bus , &tmf_device_cfg , &tmf_device) ;

esp_err_t err = i2c_master_probe(tmf_bus , TOF_ADDRESS , 50) ;

if (err==ESP_OK ){

    ESP_LOGI(TAG , "probe sucessful: " );

}
else {

    ESP_LOGE(TAG , "probe failed: %s",esp_err_to_name(err));


}



ESP_LOGI(TAG, "This is me here");


while (1){

    vTaskDelay(pdMS_TO_TICKS(1000));


}

}