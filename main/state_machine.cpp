#include "custom_netif.h"
#include "arp_chain.h"
#include "dhcp_chain.h"
#include "route_chain.h"
#include "bcmc_chain.h"
#include "debug_chain.h"

#include "state_machine.h"

static void __wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    StateMachine::instance()->wifi_event_handler(arg, base, id, data);
}

static void __ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    StateMachine::instance()->ip_event_handler(arg, base, id, data);
}

StateMachine* StateMachine::_instance = nullptr;
StateMachine* StateMachine::instance() {
    if(!_instance) {
        _instance = new StateMachine();
        if(!_instance) {
            ESP_LOGE(__func__, "State Machine failure");
        }
    }
    return _instance;
}
StateMachine::StateMachine() : _retry_connection(0){
    for(uint8_t host = 0 ; host < CONFIG_MAX_STATION ; host++) {
        memset(&_associated_hosts[host], 0, sizeof(host_info_t));
    }
    ESP_LOGI(__func__, "State Machine is ready");
}

void StateMachine::start_scan() {
    wifi_scan_config_t scan_config;

    memset(&scan_config, 0, sizeof(scan_config));
    scan_config.show_hidden = true;
    if(ESP_OK != esp_wifi_scan_start(&scan_config, false)) {
        ESP_LOGE(__func__, "Scan start is failed");
        esp_restart();
    }
    ESP_LOGI(__func__, "Scan is started");
}

void StateMachine::start_connect() {
    wifi_config_t gw_config;

    memset(&gw_config, 0, sizeof(gw_config));
    memcpy(gw_config.sta.ssid, CONFIG_GW_AP_SSID, strlen(CONFIG_GW_AP_SSID));
    memcpy(gw_config.sta.password, CONFIG_GW_AP_PASSWORD, strlen(CONFIG_GW_AP_PASSWORD));
    if(_retry_connection >= CONFIG_MAXIMUM_CONNECTION_RETRY) {
        start_scan();
        return;
    }
    if(ESP_OK != esp_wifi_set_mode(WIFI_MODE_STA)) {
        start_scan();
        return;
    }
    if(ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_STA, &gw_config)) {
        start_scan();
        return;
    }
    if (ESP_OK != esp_wifi_connect()) {
        start_scan();
        return;
    }
    _retry_connection++;
    ESP_LOGI(__func__, "Connecting to AP");
}

void StateMachine::start_service_ap(ip_event_got_ip_t *event) {
    ESP_LOGI(__func__, "Start service AP");

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.ap.ssid, CONFIG_GW_AP_SSID, strlen(CONFIG_GW_AP_SSID));
    wifi_config.ap.ssid_len = strlen(CONFIG_GW_AP_SSID);
    memcpy(wifi_config.ap.password, CONFIG_GW_AP_PASSWORD, strlen(CONFIG_GW_AP_PASSWORD));
    wifi_config.ap.max_connection = CONFIG_MAX_STATION;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if(ESP_OK != esp_netif_set_ip_info(CustomNetif::instance()->get_esp_interface(ACCESS_POINT_TYPE), (esp_netif_ip_info_t*)&(event->ip_info))) {
        esp_wifi_disconnect();
    }
    if(ESP_OK != esp_wifi_set_mode(WIFI_MODE_APSTA)){
        esp_wifi_disconnect();
    }
    if(ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)) {
        esp_wifi_disconnect();
    }
}

bool StateMachine::is_associated_sta(const uint8_t* const addr) {
    wifi_sta_list_t stations;
    if(ESP_OK != esp_wifi_ap_get_sta_list(&stations)) {
        return false;
    }
    for(uint8_t sta = 0 ; stations.num ; sta++) {
        if(memcmp(stations.sta[sta].mac, addr, NETIF_MAX_HWADDR_LEN) == 0) {
            return true;
        }
    }
    return false;
}

void StateMachine::add_associated_host(const uint8_t* const sta, const uint32_t host) {
    for(uint8_t host_idx = 0 ; host_idx < CONFIG_MAX_STATION ; host_idx++) {
        if(memcmp(_associated_hosts[host_idx].sta, sta, NETIF_MAX_HWADDR_LEN) == 0) {
            _associated_hosts[host_idx].host.u32 = host;
            ESP_LOGE(__func__, "Add host %u", host_idx);
            return;
        }
    }
    for(uint8_t host_idx = 0 ; host_idx < CONFIG_MAX_STATION ; host_idx++) {
        if(_associated_hosts[host_idx].host.u32 == 0) {
            memcpy(_associated_hosts[host_idx].sta, sta, NETIF_MAX_HWADDR_LEN);
            _associated_hosts[host_idx].host.u32 = host;
            ESP_LOGE(__func__, "Add host %u", host_idx);
            return;
        }
    }
}

void StateMachine::remove_associated_host(const uint8_t* const sta) {
    for(uint8_t host_idx = 0 ; host_idx < CONFIG_MAX_STATION ; host_idx++) {
        if(memcmp(_associated_hosts[host_idx].sta, sta, NETIF_MAX_HWADDR_LEN) == 0) {
            memset(&_associated_hosts[host_idx], 0, sizeof(host_info_t));
            ESP_LOGE(__func__, "Remove host %u", host_idx);
            return;
        }
    }
}

bool StateMachine::is_associated_host(const uint32_t host) {
    for(uint8_t host_idx = 0 ; host_idx < CONFIG_MAX_STATION ; host_idx++) {
        if(_associated_hosts[host_idx].host.u32 == host) {
            return true;
        }
    }
    return false;
}

esp_err_t StateMachine::start() {
    esp_err_t err;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    assert(CustomNetif::instance());

    if(ESP_OK != esp_wifi_init(&cfg)) {
        return ESP_FAIL;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &__wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        return ESP_FAIL;
    }
    err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &__ip_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    if(ESP_OK != esp_wifi_set_mode(WIFI_MODE_STA)) {
        return ESP_FAIL;
    }
    if(ESP_OK != esp_wifi_start()) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void StateMachine::wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    switch(id) {
    case WIFI_EVENT_WIFI_READY:
        ESP_LOGI(__func__,"SYSTEM_EVENT_WIFI_READY");
    break;

    case WIFI_EVENT_SCAN_DONE:
    {
        uint16_t num_of_ap = 0;
        wifi_ap_record_t *ap_list = NULL;

        ESP_LOGI(__func__,"SYSTEM_EVENT_SCAN_DONE");
        if(ESP_OK != esp_wifi_scan_get_ap_num(&num_of_ap)) {
            ESP_LOGE(__func__, "Scan is failed. Restart Scan");
            goto restart_scan;
        }
        ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * num_of_ap);
        if (ap_list == NULL)
        {
            ESP_LOGE(__func__, "Scan is failed. Restart Scan");
            goto restart_scan;
        }
        if(ESP_OK != esp_wifi_scan_get_ap_records(&num_of_ap, ap_list)) {
            ESP_LOGE(__func__, "Scan is failed. Restart Scan");
            goto restart_scan;
        }
        for (uint16_t ap_index = 0; ap_index < num_of_ap; ap_index++)
        {
            if (strcmp((char *)ap_list[ap_index].ssid, CONFIG_GW_AP_SSID) == 0)
            {
                ESP_LOGI(__func__, "AP is found. Connecting..");
                _retry_connection = 0;
                start_connect();
                free(ap_list);
                return;
            }
        }
        ESP_LOGE(__func__, "AP is not found. Restart Scan");
restart_scan:
        if(ap_list)
            free(ap_list);
        start_scan();
    }
    break;

    case WIFI_EVENT_STA_START:/**< ESP32 station start */
    {
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_START");
        CustomNetif::instance()->install_input_chain(STATION_TYPE);
        CustomNetif::instance()->add_chain(STATION_TYPE, arp_filter_sta, arp_process_sta);
        CustomNetif::instance()->add_chain(STATION_TYPE, dhcp_filter_sta, dhcp_process_sta);
        CustomNetif::instance()->add_chain(STATION_TYPE, bcmc_filter, bcmc_process_sta);
        CustomNetif::instance()->add_chain(STATION_TYPE, route_filter_sta, route_process_sta);
        //CustomNetif::instance()->add_chain(STATION_TYPE, nullptr, debug_process);
        start_scan();
    }
    break;

    case WIFI_EVENT_STA_STOP:/**< ESP32 station stop */
        CustomNetif::instance()->uninstall_input_chain(STATION_TYPE);
    break;

    case WIFI_EVENT_STA_CONNECTED:/**< ESP32 station connected to AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_CONNECTED");
        _retry_connection = 0;
    break;

    case WIFI_EVENT_STA_DISCONNECTED:/**< ESP32 station disconnected from AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_DISCONNECTED");
        start_connect();
    break;

    case WIFI_EVENT_STA_AUTHMODE_CHANGE:/**< the auth mode of AP connected by ESP32 station changed */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_AUTHMODE_CHANGE");
    break;

    case WIFI_EVENT_STA_WPS_ER_SUCCESS:/**< ESP32 station wps succeeds in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_SUCCESS");
    break;

    case WIFI_EVENT_STA_WPS_ER_FAILED:/**< ESP32 station wps fails in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_FAILED");
    break;

    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:/**< ESP32 station wps timeout in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_TIMEOUT");
    break;

    case WIFI_EVENT_STA_WPS_ER_PIN:/**< ESP32 station wps pin code in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_PIN");
    break;

    case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP:/*!< ESP32 station wps overlap in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP");
    break;

    case WIFI_EVENT_AP_START:/**< ESP32 soft-AP start */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_START");
        CustomNetif::instance()->install_input_chain(ACCESS_POINT_TYPE);
        CustomNetif::instance()->add_chain(ACCESS_POINT_TYPE, arp_filter_ap, arp_process_ap);
        CustomNetif::instance()->add_chain(ACCESS_POINT_TYPE, dhcp_filter_ap, dhcp_process_ap);
        CustomNetif::instance()->add_chain(ACCESS_POINT_TYPE, bcmc_filter, bcmc_process_ap);
        CustomNetif::instance()->add_chain(ACCESS_POINT_TYPE, route_filter_ap, route_process_ap);
        //CustomNetif::instance()->add_chain(ACCESS_POINT_TYPE, nullptr, debug_process);
    break;

    case WIFI_EVENT_AP_STOP:
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STOP");
        CustomNetif::instance()->uninstall_input_chain(ACCESS_POINT_TYPE);
    break;

    case WIFI_EVENT_AP_STACONNECTED:
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STACONNECTED");
    break;

    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)data;
        remove_associated_host(event->mac);
    }
    break;

    case WIFI_EVENT_AP_PROBEREQRECVED:
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_PROBEREQRECVED");
    break;

    default:
    break;
    }
}

void StateMachine::ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    switch(id) {
    case IP_EVENT_STA_GOT_IP:/**< ESP32 station got IP from connected AP */
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;

        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(__func__, "got ip:%s", ip4addr_ntoa((ip4_addr_t*)&event->ip_info.ip));
        start_service_ap(event);
    }
    break;

    case IP_EVENT_STA_LOST_IP:/**< ESP32 station lost IP and the IP is reset to 0 */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_LOST_IP");
    break;

    case IP_EVENT_AP_STAIPASSIGNED:/**< ESP32 soft-AP assign an IP to a connected station */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STAIPASSIGNED");
    break;

    case IP_EVENT_GOT_IP6:/**< ESP32 station or ap or ethernet interface v6IP addr is preferred */
        ESP_LOGI(__func__,"SYSTEM_EVENT_GOT_IP6");
    break;

    default:
    break;
    }
}
