#include "custom_netif.h"

err_t custom_input(struct pbuf *p, struct netif *inp) {
    return CustomNetif::instance()->traverse_input_chain(CustomNetif::instance()->interface_type(inp), p, inp);
}

CustomNetif* CustomNetif::_instace = new CustomNetif();

CustomNetif* CustomNetif::instance() {
    return _instace;
}

CustomNetif::CustomNetif() {
    for(uint8_t iface = 0 ; iface < TCPIP_ADAPTER_IF_MAX ; iface++) {
        for(uint8_t chain = 0 ; chain < MAXIMUM_CHAIN ; chain++) {
            _input_chain[iface][chain].filter = nullptr;
            _input_chain[iface][chain].process = nullptr;
        }
        _interfaces[iface] = nullptr;
        _original_inputs[iface] = nullptr;
    }
}

esp_err_t CustomNetif::install_input_chain(const tcpip_adapter_if_t type) {
    struct netif *interface = nullptr;

    // 0. Input function is already registered.
    if(_interfaces[type]) {
        return ESP_OK;
    }
    // 1. Find the interface.
    if(ESP_OK != tcpip_adapter_get_netif(type, (void**)&interface)) {
        ESP_LOGE(__func__, "Failed: Register input handler");
        return ESP_ERR_NOT_FOUND;
    }
    // 2. Register the interface.
    _interfaces[type] = interface;
    // 3. Initailize input chain for the interface.
    for(uint8_t i = 0 ; i < MAXIMUM_CHAIN ; i++) {
        _input_chain[type][i].filter = nullptr;
        _input_chain[type][i].process = nullptr;
    }
    // 4. Save original input function.
    _original_inputs[type] = _interfaces[type]->input;
    // 5. Resigater input function.
    _interfaces[type]->input = custom_input;
    ESP_LOGI(__func__, "Register input handler on %s%u", interface->name, interface->num);
    return ESP_OK;
}
esp_err_t CustomNetif::uninstall_input_chain(const tcpip_adapter_if_t type) {
    struct netif *interface = nullptr;

    // 0. Input function is already unregistered.
    if(!_interfaces[type]) {
        return ESP_OK;
    }
    // 1. Find the interface.
    if(ESP_OK != tcpip_adapter_get_netif(type, (void**)&interface)) {
        ESP_LOGE(__func__, "Failed: Unregister input handler");
        return ESP_ERR_NOT_FOUND;
    }
    // 2. Unresigater input function.
    interface->input = _original_inputs[type];
    // 3. Remove original input.
    _original_inputs[type] = nullptr;
    // 4. Unregister the interface
    _interfaces[type] = nullptr;
    // 5. Remove all input chains for the interface.
    for(uint8_t i = 0 ; i < MAXIMUM_CHAIN ; i++) {
        _input_chain[type][i].filter = nullptr;
        _input_chain[type][i].process = nullptr;
    }
    ESP_LOGI(__func__, "Unregister input handler on %s%u", interface->name, interface->num);
    return ESP_OK;
}

esp_err_t CustomNetif::add_chain(const tcpip_adapter_if_t type, input_chain_filter_fn filter, input_chain_process_fn process) {
    if(!_interfaces[type]) {
        return ESP_FAIL;
    }
    for(uint8_t i = 0 ; i < MAXIMUM_CHAIN ; i++) {
        if(!_input_chain[type][i].filter && !_input_chain[type][i].process) {
            _input_chain[type][i].filter = filter;
            _input_chain[type][i].process = process;
            ESP_LOGI(__func__, "Chain is added.[%hhu]", i);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

err_t CustomNetif::traverse_input_chain(const tcpip_adapter_if_t type, struct pbuf *p, struct netif *inp) {
    if(!_interfaces[type]) {
        return ERR_OK;
    }
    for(uint8_t i = 0 ; i < MAXIMUM_CHAIN ; i++) {
        if(!_input_chain[type][i].filter && _input_chain[type][i].process) {
            if(TYPE_CONSUME == _input_chain[type][i].process(type, p)) {
                pbuf_free(p);
                return ERR_OK;
            }
        } else if(_input_chain[type][i].filter && _input_chain[type][i].filter(type, p)) {
            if(TYPE_CONSUME == _input_chain[type][i].process(type, p)) {
                pbuf_free(p);
                return ERR_OK;
            }
        } else if(!_input_chain[type][i].filter && !_input_chain[type][i].process) {
            break;
        }
    }
    return _original_inputs[type](p, inp);
}

err_t CustomNetif::l3transmit(const tcpip_adapter_if_t type, struct pbuf *p, ip4_addr_t* nexthop) {
    if(!_interfaces[type]) {
        return ERR_IF;
    }
    // reset eth header.
    pbuf_header(p, -sizeof(struct eth_hdr));
    return _interfaces[type]->output(_interfaces[type], p, nexthop);
}

err_t CustomNetif::transmit(const tcpip_adapter_if_t type, struct pbuf *p) {
    if(!_interfaces[type]) {
        return ERR_IF;
    }
    memcpy(ETH(p)->src.addr, _interfaces[type]->hwaddr, NETIF_MAX_HWADDR_LEN);
    return _interfaces[type]->linkoutput(_interfaces[type], p);
}
