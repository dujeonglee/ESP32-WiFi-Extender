/**
 * @file flash.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief Flash class reading/writing data from/to nvm
 * @version 0.1
 * @date 2020-02-08
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef FLASH_H
#define FLASH_H
#include "common.h"

class Flash {
public:
    static Flash *instance();
    static Flash *_instance;
private:
    Flash();
    esp_err_t init();
public:
    esp_err_t write_u8(const char* field, uint8_t data);
    esp_err_t write_u16(const char* field, uint16_t data);
    esp_err_t write_u32(const char* field, uint32_t data);
    esp_err_t write_str(const char* field, const char* data, size_t data_length);

    esp_err_t read_u8(const char* field, uint8_t* data);
    esp_err_t read_u16(const char* field, uint16_t* data);
    esp_err_t read_u32(const char* field, uint32_t* data);
    esp_err_t read_str(const char* field, char* data, size_t* data_length);
};
#endif