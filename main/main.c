#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdbool.h>

#define I2C_PORT_NUM 0 
#define GPIO_NUM_SDA 21
#define GPIO_NUM_SCL 22
#define I2C_FREQUENCY 100000 
#define TOF_ADDRESS 0x41 
#define REG_ID 0xE3 
#define CHIP_ID 0x9E
#define REG_ENABLE 0xF8
#define ENABLE_PON (1<<2)
#define ENABLE_CPU_READY ( 1<<7 )

static const char *TAG = "MAIN";

esp_err_t read_reg(i2c_master_dev_handle_t dev ,uint8_t reg , uint8_t  *value ){

    return i2c_master_transmit_receive(dev , &reg , 1 , value ,1 ,100 ); 

}

esp_err_t write_reg( i2c_master_dev_handle_t dev , uint8_t reg , uint8_t value ) {

    uint8_t buf[2] = {reg , value };   
    return i2c_master_transmit(dev , buf , 2 , 100);


}


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

uint8_t send_reg =REG_ID ; 
uint8_t recieve_reg = 0  ;
 
err =  read_reg(tmf_device, send_reg  ,&recieve_reg  );
if (err ==ESP_OK ){

    ESP_LOGI(TAG , "Sucessfully transmitted %02X", recieve_reg);

        if (recieve_reg == CHIP_ID){
                ESP_LOGI(TAG , "Sucessfully transmitted and the ID is  %02X", recieve_reg);
            
        }else{

            ESP_LOGE(TAG,"This returned incorrect chip ID");
        }

} else{

    ESP_LOGE(TAG , "Not Transmitted sadly");

}

ESP_LOGI(TAG, "This is me here");

err = write_reg(tmf_device , REG_ENABLE , ENABLE_PON) ;

if (err == ESP_OK){

    ESP_LOGI(TAG , "Writing a register is sucessful the result ");


}
else {
    ESP_LOGI(TAG , "Write failed ");
}

uint8_t status = 0 ;

bool ready = false ;

for ( uint8_t i = 0 ; i<100 ; i++ ){

    err = read_reg(tmf_device , REG_ENABLE , &status) ;
    if (err == ESP_OK){
    if (status & ENABLE_CPU_READY ) {

    ESP_LOGI(TAG , "Device published CPU Ready");

        ready = true ; 
        break ; 
    }   } 

    else {ESP_LOGE(TAG, "Timeout Error"); 
        break ;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

if (ready == true ){
    ESP_LOGI(TAG, "CPU is ready now ");
}
else{

    ESP_LOGE(TAG, "Timeout Error");
}

while (1){

    vTaskDelay(pdMS_TO_TICKS(1000));


}

}