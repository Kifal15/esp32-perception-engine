#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdbool.h>
#include "lightranger14_firmware.h"
#include <string.h>
#include <driver/gpio.h>

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
#define REG_CMD_STAT 0x08
#define REG_APP_ID 0x00
#define BL_CMD_SPI_OFF 32
#define APP_ID_BOOTLOADER 0X80 
#define BL_CMD_W_FIFO_BOTH 69
#define REG_FIFO 0xFF
#define PIN_EN 18
#define PIN_INT 19
#define BL_CMD_START_RAM_APP 22
#define BL_CMD_LOAD_CFG_8X8 64
#define CMD_WRITE_PAGE_AND_MEASURE 20
#define REG_INT_STATUS 0xE1
#define REG_INT_ENAB 0xE2
#define REG_FIFOSTATUS 0xFA

#define CMD_STAT_OK 0x00
#define CMD_STAT_ACCEPTED 0x01

static const char *TAG = "MAIN";

esp_err_t read_reg(i2c_master_dev_handle_t dev ,uint8_t reg , uint8_t  *value ){

    return i2c_master_transmit_receive(dev , &reg , 1 , value ,1 ,100 );

}

esp_err_t write_reg( i2c_master_dev_handle_t dev , uint8_t reg , uint8_t value ) {

    uint8_t buf[2] = {reg , value };
    return i2c_master_transmit(dev , buf , 2 , 100);


}


esp_err_t read_regs(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *buf, size_t len) {

    return i2c_master_transmit_receive (dev , &reg , 1, buf , len ,100 ) ; 

}



esp_err_t send_command(i2c_master_dev_handle_t dev , uint8_t cmd , uint8_t expected ) 
{
uint8_t stat = 0 ;
esp_err_t err    = write_reg(dev , REG_CMD_STAT , cmd );

if (err != ESP_OK) {

    ESP_LOGE(TAG , "Write unsucessful please try again");
    return err ;
}


for ( int i = 0 ; i<=100 ; i++){


    err = read_reg (dev, REG_CMD_STAT ,&stat) ;
    if (err!= ESP_OK)
    {

        return err ;


    }
        if (stat==expected){
            return ESP_OK ;

        }

vTaskDelay(pdMS_TO_TICKS(1));

}

ESP_LOGE(TAG , "cmd %u: expected %02X, CMD_STAT stuck at %02X" , cmd , expected , stat );
return ESP_ERR_TIMEOUT ;


}


esp_err_t write_regs(i2c_master_dev_handle_t dev , uint8_t reg ,  const uint8_t *data, size_t len)

    {

        if (len>128){
           return ESP_ERR_INVALID_SIZE ;
        }
        uint8_t buf[129];

        buf[0] = reg ;
        memcpy(&buf[1], data , len   ) ;

        return i2c_master_transmit(dev , buf , len+1 , 100);
    }

esp_err_t sensor_reset(void){

gpio_config_t enable_config = {

    .pin_bit_mask = 1ULL << PIN_EN ,
    .mode = GPIO_MODE_OUTPUT ,
    .intr_type = GPIO_INTR_DISABLE ,
    .pull_up_en = 0 ,
    .pull_down_en = 0 ,
};


gpio_config(&enable_config);

gpio_set_level(PIN_EN , 0 );
vTaskDelay(pdMS_TO_TICKS(10));
gpio_set_level(PIN_EN , 1 );
vTaskDelay(pdMS_TO_TICKS(10));

return ESP_OK ;
}


void app_main(void){

    sensor_reset();
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


err = send_command(tmf_device , BL_CMD_SPI_OFF , CMD_STAT_OK );
if (err == ESP_OK){
ESP_LOGI(TAG,"SPI SUCESSFULLY DISABLED");
}
else{
    ESP_LOGE(TAG , "SPI NOT DISABLED");
}
uint8_t app_id = 0 ;
err = read_reg(tmf_device,REG_APP_ID ,&app_id);


if (err== ESP_OK){

    ESP_LOGI(TAG , "App ID was read ");
ESP_LOGI(TAG," THE APP ID IS %02X" ,app_id);


}

uint32_t fw_size = LIGHTRANGER14_IMAGE_FINISH - LIGHTRANGER14_IMAGE_START ;
uint32_t fw_words = (fw_size+3)/4 ;

uint8_t sending_array [8] = {

    BL_CMD_W_FIFO_BOTH ,
    6 ,
    LIGHTRANGER14_IMAGE_START >> 0 & 0xFF,
    LIGHTRANGER14_IMAGE_START >> 8 & 0xFF,
    LIGHTRANGER14_IMAGE_START >> 16 & 0xFF,
    LIGHTRANGER14_IMAGE_START >> 24 & 0xFF,
    fw_words >> 0 & 0xFF ,
    fw_words >> 8 & 0xFF ,

};

ESP_LOG_BUFFER_HEX(TAG , sending_array , 8 );
err = write_regs(tmf_device , REG_CMD_STAT , sending_array , 8 );

if (err  == ESP_OK){

ESP_LOGI(TAG , "Array was sent ok here is the response ");
uint8_t stat = 0 ;
for (uint8_t i = 0 ; i< 100 ; i++){
    err = read_reg(tmf_device , REG_CMD_STAT , &stat) ;
    ESP_LOGI(TAG , " Here is the REG_CMD_STAT , Status of the CPU  %02x ", stat);


vTaskDelay(pdMS_TO_TICKS(1));
    if (stat==0){
        ESP_LOGI(TAG,"FOUND 0 ");
        break ;
    }
}
}
else {

ESP_LOGE(TAG , "Error sending array");

}

uint32_t offset = 0 ;


while (offset<fw_size){

    size_t chunk = 128 ;


    if (offset +chunk >fw_size)
{
chunk = fw_size - offset ;
}

    err = write_regs(tmf_device , REG_FIFO , &lightranger14_image[offset] , chunk );

    if (err!= ESP_OK){

        ESP_LOGE(TAG , " The upload died here at this offset %lu  " , offset) ;
        break ;

    }

    offset += chunk ;




}


ESP_LOGI(TAG, "uploaded %lu bytes", offset);
err = send_command(tmf_device , BL_CMD_START_RAM_APP , CMD_STAT_OK );

if (err != ESP_OK){

    ESP_LOGE (TAG, " BL_CMD_START_RAM_APP NOT SENT PROPERLY ");
}

for (uint8_t i = 0 ; i < 100 ; i++ ){


err = read_reg(tmf_device , REG_APP_ID , &app_id) ;

if (err == ESP_OK ){
    ESP_LOGI (TAG , " APP ID RETRIVED IS %02x " , app_id ) ;
}

else{

ESP_LOGE(TAG , " APP ID IS NOT SET YET / NOT RETRIVED YET ");


}

vTaskDelay(pdMS_TO_TICKS(10));
if (app_id == 0x01){
    break;
}

}

err= send_command(tmf_device , BL_CMD_LOAD_CFG_8X8 , CMD_STAT_OK );

if (err != ESP_OK){

    ESP_LOGE (TAG, " ISSUE IN BL_CMD_LOAD_CFG_8X8 ");
}
err = write_reg(tmf_device, REG_INT_ENAB, 0x01);   // 0x01 = results interrupt

// Measurement runs continuously, so CMD_STAT settles at ACCEPTED, not OK.
err = send_command(tmf_device , CMD_WRITE_PAGE_AND_MEASURE , CMD_STAT_ACCEPTED );

if (err != ESP_OK){

    ESP_LOGE (TAG, " ISSUE IN CMD_WRITE_PAGE_AND_MEASURE ");
}
else {
    ESP_LOGI (TAG, " MEASUREMENT STARTED ");
}

uint8_t int_stat = 0 ;

for (uint8_t i = 0 ; i < 100 ; i++ ){


err = read_reg(tmf_device , REG_INT_STATUS , &int_stat) ;

if (err != ESP_OK ){

ESP_LOGE(TAG , " int stat IS NOT SET YET / NOT RETRIVED YET ");

}
else if (int_stat != 0){

    ESP_LOGI (TAG , " FRAME READY, INT STAT IS %02x " , int_stat ) ;

    err = write_reg(tmf_device , REG_INT_STATUS , int_stat ) ;

    if (err != ESP_OK){
        ESP_LOGE(TAG , " FAILED TO CLEAR INT STATUS ");
    }

    break ;
}

vTaskDelay(pdMS_TO_TICKS(10));

}

uint8_t frame[225] ; 




while (1){
read_regs (tmf_device  ,REG_FIFOSTATUS , frame ,225 );

ESP_LOG_BUFFER_HEX(TAG, frame, 225);

    vTaskDelay(pdMS_TO_TICKS(1000));


}

}