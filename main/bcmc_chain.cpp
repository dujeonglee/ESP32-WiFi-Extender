#include "custom_netif.h"
#include "bcmc_chain.h"

bool bcmc_filter(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct netif* iface = CustomNetif::instance()->get_interface(type);

    if(ETHTYPE_IP == ntohs(ETH(p)->type) && IP_PROTO_UDP == IP4(p)->_proto && (68 == ntohs(UDP4(p)->dest) || 67 == ntohs(UDP4(p)->dest))) {
        // DHCP will be handled in DHCP chain.
        return false;
    }
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
    if(!CustomNetif::instance()->get_interface(TCPIP_ADAPTER_IF_STA)) {
        pbuf_free(p);
        return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
    }
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_STA, p)) {
        ESP_LOGE(__func__, "Cannot relay bcmc to STA");
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}

pkt_fate_t bcmc_process_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(!CustomNetif::instance()->get_interface(TCPIP_ADAPTER_IF_AP)) {
        pbuf_free(p);
        return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
    }
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_AP, p)) {
        ESP_LOGE(__func__, "Cannot relay bcmc to AP");
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}
