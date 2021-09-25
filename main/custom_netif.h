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
    TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN = 0,
    TYPE_FORWARD_PACKET_TO_NEXT_INPUT_CHAIN
};

enum interface_t {
    STATION_TYPE = 0,
    ACCESS_POINT_TYPE,
    INVALID_TYPE,
    MAX_TYPE = INVALID_TYPE
};

typedef bool (*input_chain_filter_fn)(const interface_t type, struct pbuf *p);
typedef pkt_fate_t (*input_chain_process_fn)(const interface_t type, struct pbuf *p);

struct input_chain_t {
    input_chain_filter_fn filter;
    input_chain_process_fn process;
};

class CustomNetif {
private:
    static CustomNetif* _instace;
    CustomNetif();

    esp_netif_t *_esp_interfaces[MAX_TYPE];
    struct input_chain_t _input_chain[MAX_TYPE][MAXIMUM_CHAIN];
    struct netif* _interfaces[MAX_TYPE];
    netif_input_fn _original_inputs[MAX_TYPE];
public:
    static CustomNetif* instance();
    inline interface_t interface_type(const struct netif * const inp) {
        if(_interfaces[ACCESS_POINT_TYPE] == inp) {
            return ACCESS_POINT_TYPE;
        } else if(_interfaces[STATION_TYPE] == inp) {
            return STATION_TYPE;
        } else {
            return INVALID_TYPE;
        }
    }
    inline struct eth_addr* interface_address(const interface_t type) {
        if(!_interfaces[type]) {
            return nullptr;
        }
        return (struct eth_addr*)_interfaces[type]->hwaddr;
    }
    inline struct netif* get_interface(const interface_t type) {
        return _interfaces[type];
    }
    inline esp_netif_t* get_esp_interface(const interface_t type) {
        return _esp_interfaces[type];
    }
    esp_err_t install_input_chain(const interface_t type);
    esp_err_t uninstall_input_chain(const interface_t type);
    esp_err_t add_chain(const interface_t type, input_chain_filter_fn filter, input_chain_process_fn process);
    err_t traverse_input_chain(const interface_t type, struct pbuf *p, struct netif *inp);

    err_t l3transmit(const interface_t type, struct pbuf *p, ip4_addr_t* nexthop = nullptr);
    err_t transmit(const interface_t type, struct pbuf *p);
};

#endif
