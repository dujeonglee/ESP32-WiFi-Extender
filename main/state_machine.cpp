#include "custom_netif.h"
#include "arp_chain.h"
#include "dhcp_chain.h"
#include "route_chain.h"
#include "bcmc_chain.h"
#include "debug_chain.h"

#include "state_machine.h"

static esp_err_t system_event_handler(void *ctx, system_event_t *event) {
    (void)ctx;
    StateMachine::instance()->event_handler(event);
    return ESP_OK;
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

void StateMachine::start_service_ap(system_event_t* event) {
    ESP_LOGI(__func__, "Start service AP");

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.ap.ssid, CONFIG_GW_AP_SSID, strlen(CONFIG_GW_AP_SSID));
    wifi_config.ap.ssid_len = strlen(CONFIG_GW_AP_SSID);
    memcpy(wifi_config.ap.password, CONFIG_GW_AP_PASSWORD, strlen(CONFIG_GW_AP_PASSWORD));
    wifi_config.ap.max_connection = CONFIG_MAX_STATION;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if(ESP_OK != tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP)) {
        esp_wifi_disconnect();
    }
    if(ESP_OK != tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &(event->event_info.got_ip.ip_info))) {
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
    tcpip_adapter_init();
    if(ESP_OK != esp_event_loop_init(system_event_handler, NULL)) {
        return ESP_FAIL;
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(ESP_OK != esp_wifi_init(&cfg)) {
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
void StateMachine::event_handler(system_event_t* event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_WIFI_READY:/**< ESP32 WiFi ready */
        ESP_LOGI(__func__,"SYSTEM_EVENT_WIFI_READY");
    break;
    case SYSTEM_EVENT_SCAN_DONE:/**< ESP32 finish scanning AP */
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
    case SYSTEM_EVENT_STA_START:/**< ESP32 station start */
    {
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_START");
        CustomNetif::instance()->install_input_chain(TCPIP_ADAPTER_IF_STA);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_STA, arp_filter_sta, arp_process_sta);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_STA, dhcp_filter_sta, dhcp_process_sta);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_STA, bcmc_filter, bcmc_process_sta);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_STA, route_filter_sta, route_process_sta);
        //CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_STA, nullptr, debug_process);
        start_scan();
    }
    break;
    case SYSTEM_EVENT_STA_STOP:/**< ESP32 station stop */
        CustomNetif::instance()->uninstall_input_chain(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:/**< ESP32 station connected to AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_CONNECTED");
        _retry_connection = 0;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:/**< ESP32 station disconnected from AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_DISCONNECTED");
        start_connect();
        break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:/**< the auth mode of AP connected by ESP32 station changed */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_AUTHMODE_CHANGE");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:/**< ESP32 station got IP from connected AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(__func__, "got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        start_service_ap(event);
        break;
    case SYSTEM_EVENT_STA_LOST_IP:/**< ESP32 station lost IP and the IP is reset to 0 */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_LOST_IP");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:/**< ESP32 station wps succeeds in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_SUCCESS");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:/**< ESP32 station wps fails in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_FAILED");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:/**< ESP32 station wps timeout in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_TIMEOUT");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:/**< ESP32 station wps pin code in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_PIN");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP:/*!< ESP32 station wps overlap in enrollee mode */
        ESP_LOGI(__func__,"SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP");
        break;
    case SYSTEM_EVENT_AP_START:/**< ESP32 soft-AP start */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_START");
        CustomNetif::instance()->install_input_chain(TCPIP_ADAPTER_IF_AP);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_AP, arp_filter_ap, arp_process_ap);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_AP, dhcp_filter_ap, dhcp_process_ap);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_STA, bcmc_filter, bcmc_process_ap);
        CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_AP, route_filter_ap, route_process_ap);
        //CustomNetif::instance()->add_chain(TCPIP_ADAPTER_IF_AP, nullptr, debug_process);
        break;
    case SYSTEM_EVENT_AP_STOP:/**< ESP32 soft-AP stop */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STOP");
        CustomNetif::instance()->uninstall_input_chain(TCPIP_ADAPTER_IF_AP);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:/**< a station connected to ESP32 soft-AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STACONNECTED");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:/**< a station disconnected from ESP32 soft-AP */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STADISCONNECTED");
        remove_associated_host(event->event_info.sta_disconnected.mac);
        break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:/**< ESP32 soft-AP assign an IP to a connected station */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_STAIPASSIGNED");
        break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:/**< Receive probe request packet in soft-AP interface */
        ESP_LOGI(__func__,"SYSTEM_EVENT_AP_PROBEREQRECVED");
        break;
    case SYSTEM_EVENT_GOT_IP6:/**< ESP32 station or ap or ethernet interface v6IP addr is preferred */
        ESP_LOGI(__func__,"SYSTEM_EVENT_GOT_IP6");
        break;
    default:
        break;
    }
}
