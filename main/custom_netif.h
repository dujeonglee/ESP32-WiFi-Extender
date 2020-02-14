/**
 * @file custom_netif.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief Custom netif interface
 * @version 0.1
 * @date 2020-02-08
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef CUSTOMNETIF_H
#define CUSTOMNETIF_H
#include "common.h"

#define MAXIMUM_CHAIN      (10)

enum pkt_fate_t {
    TYPE_CONSUME = 0,
    TYPE_FORWARD
};

typedef bool (*input_chain_filter_fn)(const tcpip_adapter_if_t type, struct pbuf *p);
typedef pkt_fate_t (*input_chain_process_fn)(const tcpip_adapter_if_t type, struct pbuf *p);

struct input_chain_t {
    input_chain_filter_fn filter;
    input_chain_process_fn process;
};

class CustomNetif {
private:
    static CustomNetif* _instace;
    CustomNetif();

    struct input_chain_t _input_chain[TCPIP_ADAPTER_IF_MAX][MAXIMUM_CHAIN];
    struct netif* _interfaces[TCPIP_ADAPTER_IF_MAX];
    netif_input_fn _original_inputs[TCPIP_ADAPTER_IF_MAX];
public:
    static CustomNetif* instance();
    inline tcpip_adapter_if_t interface_type(const struct netif * const inp) {
        if(_interfaces[TCPIP_ADAPTER_IF_AP] == inp) {
            return TCPIP_ADAPTER_IF_AP;
        } else if(_interfaces[TCPIP_ADAPTER_IF_STA] == inp) {
            return TCPIP_ADAPTER_IF_STA;
        } else {
            return TCPIP_ADAPTER_IF_ETH;
        }
    }
    inline struct eth_addr* interface_address(const tcpip_adapter_if_t type) {
        if(!_interfaces[type]) {
            return nullptr;
        }
        return (struct eth_addr*)_interfaces[type]->hwaddr;
    }
    inline struct netif* get_interface(const tcpip_adapter_if_t type) {
        return _interfaces[type];
    }
    esp_err_t install_input_chain(const tcpip_adapter_if_t type);
    esp_err_t uninstall_input_chain(const tcpip_adapter_if_t type);
    esp_err_t add_chain(const tcpip_adapter_if_t type, input_chain_filter_fn filter, input_chain_process_fn process);
    err_t traverse_input_chain(const tcpip_adapter_if_t type, struct pbuf *p, struct netif *inp);

    err_t l3transmit(const tcpip_adapter_if_t type, struct pbuf *p, ip4_addr_t* nexthop = nullptr);
    err_t transmit(const tcpip_adapter_if_t type, struct pbuf *p);
};

#endif
