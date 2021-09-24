/**
 * @file state_machine.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief State machine of wifi extender
 * @version 0.1
 * @date 2020-02-08
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef STATEMACHINE_H
#define STATEMACHINE_H
#include "common.h"

struct host_info_t {
    uint8_t sta[6];
    union {
        uint8_t u8[4];
        uint32_t u32;
    } host;
};

class StateMachine {
private:
    static StateMachine* _instance;
    StateMachine();
    void start_scan();
    void start_connect();
    void start_service_ap(ip_event_got_ip_t *event);

    uint8_t _retry_connection;
    host_info_t _associated_hosts[CONFIG_MAX_STATION];
public:
    static StateMachine* instance();
    esp_err_t start();
    void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data);
    void ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data);
    bool is_associated_sta(const uint8_t* const addr);
    void add_associated_host(const uint8_t* const sta, const uint32_t addr);
    void remove_associated_host(const uint8_t* const sta);
    bool is_associated_host(const uint32_t host);
};
#endif
