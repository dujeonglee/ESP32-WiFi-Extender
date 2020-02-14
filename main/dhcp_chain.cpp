#include "custom_netif.h"
#include "state_machine.h"
#include "dhcp_chain.h"

bool dhcp_filter_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(ETH(p)->dest.addr[0] != 0xff || ETH(p)->dest.addr[1] != 0xff || ETH(p)->dest.addr[2] != 0xff || ETH(p)->dest.addr[3] != 0xff || ETH(p)->dest.addr[4] != 0xff || ETH(p)->dest.addr[5] != 0xff) {
        return false;
    }
    if(ETHTYPE_IP != ntohs(ETH(p)->type) || IP_PROTO_UDP != IP4(p)->_proto || 67 != ntohs(UDP4(p)->dest)) {
        return false;
    }
    return true;
}

pkt_fate_t dhcp_process_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
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
    //DHCP(new_p)->flags = htons(0x8000);
    //UDP4(new_p)->chksum = 0;
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_STA, new_p)) {
        ESP_LOGE(__func__, "Cannot relay dhcp to STA");
        pbuf_free(new_p);
        return TYPE_FORWARD;
    }
    pbuf_free(new_p);
    return TYPE_CONSUME;
}

bool dhcp_filter_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(ETHTYPE_IP != ntohs(ETH(p)->type) || IP_PROTO_UDP != IP4(p)->_proto || 68 != ntohs(UDP4(p)->dest)) {
        return false;
    }
    if(!StateMachine::instance()->is_associated_sta(DHCP(p)->chaddr)) {
        return false;
    }
    return true;
}

pkt_fate_t dhcp_process_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct pbuf *new_p = nullptr;
    uint8_t *p_option = nullptr;
    bool ack = false;
    uint8_t *router_address = NULL;

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
    //DHCP(new_p)->flags = htons(0x8000);
    //UDP4(new_p)->chksum = 0;
    if(ERR_OK != CustomNetif::instance()->l3transmit(TCPIP_ADAPTER_IF_AP, new_p)) {
        ESP_LOGE(__func__, "Cannot relay dhcp to AP");
        pbuf_free(new_p);
        return TYPE_FORWARD;
    }
    pbuf_free(new_p);

    p_option = (uint8_t *)DHCP(p)->options;
    while (p_option[0] != 0xff)
    {
        if (p_option[0] == 53 && p_option[1] == 1 && p_option[2] == 5)
        {
            // Ack
            ack = true;
        }
        if (p_option[0] == 3)
        {
            // Router option;
            router_address = p_option + 2;
        }
        if (ack && router_address)
        {
            break;
        }
        else
        {
            p_option = p_option + sizeof(uint8_t) + sizeof(uint8_t) + p_option[1];
        }
    }
    if(p_option[0] != 0xff) {
        ESP_LOGE(__func__, "DHCP ACK %hhu.%hhu.%hhu.%hhu", ((uint8_t*)&(DHCP(p)->yiaddr.addr))[0], ((uint8_t*)&(DHCP(p)->yiaddr.addr))[1], ((uint8_t*)&(DHCP(p)->yiaddr.addr))[2], ((uint8_t*)&(DHCP(p)->yiaddr.addr))[3]);
        StateMachine::instance()->add_associated_host(DHCP(p)->chaddr, DHCP(p)->yiaddr.addr);
    }
    return TYPE_CONSUME;
}
