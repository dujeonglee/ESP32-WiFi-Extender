#include "flash.h"

Flash *Flash::instance() {
    if(!_instance) {
        _instance = new Flash();
    }
    return _instance;
}
Flash *Flash::_instance = nullptr;

Flash::Flash() {
    ESP_ERROR_CHECK(init());
}

esp_err_t Flash::init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    return ret;
}
esp_err_t Flash::write_u8(const char* field, uint8_t data) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_u8(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}
esp_err_t Flash::write_u16(const char* field, uint16_t data) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_u16(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}
esp_err_t Flash::write_u32(const char* field, uint32_t data) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_u32(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}
esp_err_t Flash::write_str(const char* field, const char* data, size_t data_length) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_str(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}

esp_err_t Flash::read_u8(const char* field, uint8_t* data) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_u8(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}
esp_err_t Flash::read_u16(const char* field, uint16_t* data) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_u16(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}
esp_err_t Flash::read_u32(const char* field, uint32_t* data) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_u32(handle, field, data);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;
}
esp_err_t Flash::read_str(const char* field, char* data, size_t* data_length) {
    nvs_handle handle;
    esp_err_t ret;
    
    ret = init();
    if(ret != ESP_OK) {
        return ret;
    }
    ret = nvs_open("INFO", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_str(handle, field, data, data_length);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    return ret;    
}
