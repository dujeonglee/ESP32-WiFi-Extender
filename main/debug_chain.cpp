#include "custom_netif.h"
#include "debug_chain.h"

pkt_fate_t debug_process(const tcpip_adapter_if_t type, struct pbuf *p) {
    ESP_LOGI(__func__, "Packet is received from %u", type);

    ESP_LOGI(__func__, "ETH %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx -> %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx [%04hx]", 
                        ETH(p)->src.addr[0], ETH(p)->src.addr[1], ETH(p)->src.addr[2], ETH(p)->src.addr[3], ETH(p)->src.addr[4], ETH(p)->src.addr[5],
                        ETH(p)->dest.addr[0], ETH(p)->dest.addr[1], ETH(p)->dest.addr[2], ETH(p)->dest.addr[3], ETH(p)->dest.addr[4], ETH(p)->dest.addr[5],
                        ntohs(ETH(p)->type));

    if( ETHTYPE_IP == ntohs(ETH(p)->type)) {
        ESP_LOGI(__func__, "IP %hhu.%hhu.%hhu.%hhu -> %hhu.%hhu.%hhu.%hhu", 
                        ((uint8_t*)(&IP4(p)->src.addr))[0], ((uint8_t*)(&IP4(p)->src.addr))[1], ((uint8_t*)(&IP4(p)->src.addr))[2], ((uint8_t*)(&IP4(p)->src.addr))[3],
                        ((uint8_t*)(&IP4(p)->dest.addr))[0], ((uint8_t*)(&IP4(p)->dest.addr))[1], ((uint8_t*)(&IP4(p)->dest.addr))[2], ((uint8_t*)(&IP4(p)->dest.addr))[3]);
        if(IP_PROTO_TCP == IP4(p)->_proto) {
            ESP_LOGI(__func__, "TCP %hhu -> %hhu", ntohs(TCP4(p)->src), ntohs(TCP4(p)->dest));
        }
        if(IP_PROTO_UDP == IP4(p)->_proto) {
            ESP_LOGI(__func__, "UDP %hhu -> %hhu", ntohs(UDP4(p)->src), ntohs(UDP4(p)->dest));
        }
    }
    pbuf_free(p);
    return TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN;
}
