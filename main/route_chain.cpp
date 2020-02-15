#include "custom_netif.h"
#include "state_machine.h"
#include "route_chain.h"

bool route_filter_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    return ETHTYPE_IP == ntohs(ETH(p)->type);
}

pkt_fate_t route_process_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
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
    const err_t err = CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_STA, new_p);
    pbuf_free(new_p);
    if(ERR_OK != err) {
        ESP_LOGI(__func__,"Could not forward packet on STA");
    }
    return TYPE_CONSUME;
}

bool route_filter_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    return (ETHTYPE_IP == ntohs(ETH(p)->type)) && StateMachine::instance()->is_associated_host(IP4(p)->dest.addr);
}

pkt_fate_t route_process_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
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
    const err_t err = CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_AP, new_p);
    pbuf_free(new_p);
    if(ERR_OK != err) {
        ESP_LOGI(__func__,"Could not forward packet on AP");
    }
    return TYPE_CONSUME;
}
