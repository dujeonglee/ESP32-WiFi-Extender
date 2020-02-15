#include "custom_netif.h"
#include "state_machine.h"
#include "arp_chain.h"

static err_t transmit_proxy_arp(const tcpip_adapter_if_t type, const struct eth_addr *ethsrc_addr,
                 const struct eth_addr *ethdst_addr,
                 const struct eth_addr *hwsrc_addr, const ip4_addr_t *ipsrc_addr,
                 const struct eth_addr *hwdst_addr, const ip4_addr_t *ipdst_addr,
                 const u16_t opcode) {
    struct pbuf *p;
    err_t result = ERR_OK;
    struct eth_hdr *ethhdr;
    struct etharp_hdr *hdr;

    /* allocate a pbuf for the outgoing ARP request packet */
    p = pbuf_alloc(PBUF_RAW_TX, /*SIZEOF_ETHARP_PACKET_TX*/ 14+2+28, PBUF_RAM);
    /* could allocate a pbuf for an ARP request? */
    if (p == NULL) {
        return ERR_MEM;
    }

    ethhdr = (struct eth_hdr *)p->payload;
    hdr = (struct etharp_hdr *)((u8_t *)ethhdr + SIZEOF_ETH_HDR);
    hdr->opcode = htons(opcode);

    /* Write the ARP MAC-Addresses */
    ETHADDR16_COPY(&hdr->shwaddr, hwsrc_addr);
    ETHADDR16_COPY(&hdr->dhwaddr, hwdst_addr);
    /* Copy struct ip4_addr2 to aligned ip4_addr, to support compilers without
   * structure packing. */
    IPADDR2_COPY(&hdr->sipaddr, ipsrc_addr);
    IPADDR2_COPY(&hdr->dipaddr, ipdst_addr);

    hdr->hwtype = PP_HTONS(1);
    hdr->proto = PP_HTONS(ETHTYPE_IP);
    /* set hwlen and protolen */
    hdr->hwlen = ETH_HWADDR_LEN;
    hdr->protolen = sizeof(ip4_addr_t);

    ethhdr->type = PP_HTONS(ETHTYPE_ARP);

    ETHADDR16_COPY(&ethhdr->dest, ethdst_addr);
    ETHADDR16_COPY(&ethhdr->src, ethsrc_addr);

    /* send ARP query */
    result = CustomNetif::instance()->transmit(type, p);
    /* free ARP query packet */
    pbuf_free(p);
    p = NULL;
    /* could not allocate pbuf for ARP request */
    return result;
}

bool arp_filter_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(ETH(p)->dest.addr[0] != 0xff || ETH(p)->dest.addr[1] != 0xff || ETH(p)->dest.addr[2] != 0xff || ETH(p)->dest.addr[3] != 0xff || ETH(p)->dest.addr[4] != 0xff || ETH(p)->dest.addr[5] != 0xff) {
        return false;
    }
    if(ETHTYPE_ARP != ntohs(ETH(p)->type)) {
        return false;
    }
    if(HWTYPE_ETHERNET != ntohs(ARP(p)->hwtype)) {
        return false;
    }
    if(ETHTYPE_IP != ntohs(ARP(p)->proto)) {
        return false;
    }
    if(ETH_HWADDR_LEN != ARP(p)->hwlen) {
        return false;
    }
    if(sizeof(ip4_addr_t) != ARP(p)->protolen) {
        return false;
    }
    if(ARP_REQUEST != ntohs(ARP(p)->opcode)) {
        return false;
    }
    return true;
}

pkt_fate_t arp_process_ap(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(!CustomNetif::instance()->interface_address(type)) {
        pbuf_free(p);
        return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
    }
    const err_t err = transmit_proxy_arp(type,
                   CustomNetif::instance()->interface_address(type),
                   &(ARP(p)->shwaddr),
                   CustomNetif::instance()->interface_address(type),
                   (ip4_addr_t *)(ARP(p)->dipaddr.addrw),
                   &(ARP(p)->shwaddr),
                   (ip4_addr_t *)(ARP(p)->sipaddr.addrw),
                   ARP_REPLY);
    if(ERR_OK != err) {
        ESP_LOGE(__func__, "Cannot send ARP response");
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}

bool arp_filter_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    struct netif* iface = CustomNetif::instance()->get_interface(type);
    if(ETH(p)->dest.addr[0] != 0xff || ETH(p)->dest.addr[1] != 0xff || ETH(p)->dest.addr[2] != 0xff || ETH(p)->dest.addr[3] != 0xff || ETH(p)->dest.addr[4] != 0xff || ETH(p)->dest.addr[5] != 0xff) {
        return false;
    }
        if(ETHTYPE_ARP != ntohs(ETH(p)->type)) {
        return false;
    }
    if(HWTYPE_ETHERNET != ntohs(ARP(p)->hwtype)) {
        return false;
    }
    if(ETHTYPE_IP != ntohs(ARP(p)->proto)) {
        return false;
    }
    if(ETH_HWADDR_LEN != ARP(p)->hwlen) {
        return false;
    }
    if(sizeof(ip4_addr_t) != ARP(p)->protolen) {
        return false;
    }
    if(ARP_REQUEST != ntohs(ARP(p)->opcode)) {
        return false;
    }
    if(!(StateMachine::instance()->is_associated_host(*((uint32_t*)ARP(p)->dipaddr.addrw))) &&
       (iface->ip_addr.u_addr.ip4.addr != *((uint32_t*)ARP(p)->dipaddr.addrw))) {
        return false;
    }
    return true;
}

pkt_fate_t arp_process_sta(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(!CustomNetif::instance()->interface_address(type)) {
        pbuf_free(p);
        return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
    }
    const err_t err = transmit_proxy_arp(type,
                   CustomNetif::instance()->interface_address(type),
                   &(ARP(p)->shwaddr),
                   CustomNetif::instance()->interface_address(type),
                   (ip4_addr_t *)(ARP(p)->dipaddr.addrw),
                   &(ARP(p)->shwaddr),
                   (ip4_addr_t *)(ARP(p)->sipaddr.addrw),
                   ARP_REPLY);
    if(ERR_OK != err) {
        ESP_LOGE(__func__, "Cannot send ARP response");
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}
