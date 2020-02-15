#include "custom_netif.h"
#include "state_machine.h"
#include "route_chain.h"

bool route_filter_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct netif *iface = CustomNetif::instance()->get_interface(type);
    if(IP4(p)->dest.addr == iface->ip_addr.u_addr.ip4.addr) {
        return false;
    }
    return ETHTYPE_IP == ntohs(ETH(p)->type);
}

pkt_fate_t route_process_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(!CustomNetif::instance()->get_interface(TCPIP_ADAPTER_IF_STA)) {
        pbuf_free(p);
        return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
    }
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_STA, p)) {
        ESP_LOGI(__func__,"Could not forward packet on STA");
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}

bool route_filter_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct netif *iface = CustomNetif::instance()->get_interface(type);
    if(IP4(p)->dest.addr == iface->ip_addr.u_addr.ip4.addr) {
        return false;
    }
    return (ETHTYPE_IP == ntohs(ETH(p)->type)) && StateMachine::instance()->is_associated_host(IP4(p)->dest.addr);
}

pkt_fate_t route_process_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(!CustomNetif::instance()->get_interface(TCPIP_ADAPTER_IF_AP)) {
        pbuf_free(p);
        return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
    }
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_AP, p)) {
        ESP_LOGI(__func__,"Could not forward packet on AP");
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}
