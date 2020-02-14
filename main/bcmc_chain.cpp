#include "custom_netif.h"
#include "bcmc_chain.h"

bool bcmc_filter(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct netif* iface = CustomNetif::instance()->get_interface(type);

    if((IP4(p)->dest.addr & iface->netmask.u_addr.ip4.addr) == (iface->ip_addr.u_addr.ip4.addr & iface->netmask.u_addr.ip4.addr)) {
        // Link local broadcast. X.X.X.255
        return ((((uint8_t*)&(IP4(p)->dest.addr))[3] & 0xff) == 0xff);
    } else if (((((uint8_t*)&(IP4(p)->dest.addr))[0] & 0xf0) == 0xe0) ||
                IP4(p)->dest.addr == 0xffffffff) {
        // Multicast or Broadcast
        return true;
    } else {
        // Unicast
        return false;
    }
}

pkt_fate_t bcmc_process_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct pbuf *new_p = nullptr;

    if(!CustomNetif::instance()->get_interface(TCPIP_ADAPTER_IF_STA)) {
        return TYPE_FORWARD;
    }

    new_p = pbuf_alloc(PBUF_RAW_TX, p->tot_len, PBUF_RAM);
    if(!new_p) {
        ESP_LOGE(__func__, "pbuf allocation is failed.");
        return TYPE_FORWARD;
    }
    if(ERR_ARG == pbuf_copy(new_p, p)) {
        ESP_LOGE(__func__, "pbuf copy is failed.");
        pbuf_free(new_p);
        new_p = NULL;
        return TYPE_FORWARD;
    }
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_STA, new_p)) {
        ESP_LOGE(__func__, "Cannot relay bcmc to STA");
        pbuf_free(new_p);
        return TYPE_FORWARD;
    }
    pbuf_free(new_p);
    return TYPE_CONSUME;
}

pkt_fate_t bcmc_process_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct pbuf *new_p = nullptr;

    if(!CustomNetif::instance()->get_interface(TCPIP_ADAPTER_IF_AP)) {
        return TYPE_FORWARD;
    }

    new_p = pbuf_alloc(PBUF_RAW_TX, p->tot_len, PBUF_RAM);
    if(!new_p) {
        ESP_LOGE(__func__, "pbuf allocation is failed.");
        return TYPE_FORWARD;
    }
    if(ERR_ARG == pbuf_copy(new_p, p)) {
        ESP_LOGE(__func__, "pbuf copy is failed.");
        pbuf_free(new_p);
        new_p = NULL;
        return TYPE_FORWARD;
    }
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_AP, new_p)) {
        ESP_LOGE(__func__, "Cannot relay bcmc to AP");
        pbuf_free(new_p);
        return TYPE_FORWARD;
    }
    pbuf_free(new_p);
    return TYPE_CONSUME;
}
