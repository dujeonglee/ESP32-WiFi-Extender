#include "custom_netif.h"

err_t custom_input(struct pbuf *p, struct netif *inp) {
    return CustomNetif::instance()->traverse_input_chain(CustomNetif::instance()->interface_type(inp), p, inp);
}

CustomNetif* CustomNetif::_instace = nullptr;

CustomNetif* CustomNetif::instance() {
    if (!_instace) {

        _instace = new CustomNetif();
        assert(_instace);
    }
    return _instace;
}

CustomNetif::CustomNetif() {
    assert(ESP_OK == esp_netif_init());
    assert(ESP_OK == esp_event_loop_create_default());

    for(uint8_t iface = STATION_TYPE ; iface < INVALID_TYPE ; iface++) {
        esp_netif_t *esp_nif;

        switch (iface) {
            case STATION_TYPE:
            {
                esp_nif = esp_netif_create_default_wifi_sta();
            }
            break;

            case ACCESS_POINT_TYPE:
            {
                esp_netif_inherent_config_t netif_cfg;

                memcpy(&netif_cfg, ESP_NETIF_BASE_DEFAULT_WIFI_AP, sizeof(netif_cfg));
                netif_cfg.flags  = ESP_NETIF_FLAG_AUTOUP; /* DO NOT RUN DHCP SERVER */
                esp_netif_config_t cfg_ap = {
                        .base = &netif_cfg,
                        .driver = nullptr,
                        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_AP,
                };
                esp_nif = esp_netif_new(&cfg_ap);
                assert(esp_nif);
                ESP_ERROR_CHECK(esp_netif_attach_wifi_ap(esp_nif));
                ESP_ERROR_CHECK(esp_wifi_set_default_wifi_ap_handlers());
                // ...and stop DHCP server to be compatible with former tcpip_adapter (to keep the ESP_NETIF_DHCP_STOPPED state)
                ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_nif));
            }
            break;

            default:
            esp_nif = nullptr;
            break;
        }
        assert(esp_nif);

        _esp_interfaces[iface] = esp_nif;

        for(uint8_t chain = 0 ; chain < MAXIMUM_CHAIN ; chain++) {
            _input_chain[iface][chain].filter = nullptr;
            _input_chain[iface][chain].process = nullptr;
        }
        _interfaces[iface] = nullptr;
        _original_inputs[iface] = nullptr;
    }
}

esp_err_t CustomNetif::install_input_chain(const interface_t type) {
    struct netif *interface = nullptr;

    // 0. Input function is already registered.
    if(_interfaces[type]) {
        return ESP_OK;
    }
    // 1. Find the interface.
    interface = _esp_interfaces[type]->lwip_netif;
    if(!interface) {
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
esp_err_t CustomNetif::uninstall_input_chain(const interface_t type) {
    struct netif *interface = nullptr;

    // 0. Input function is already unregistered.
    if(!_interfaces[type]) {
        return ESP_OK;
    }
    // 1. Find the interface.
    interface = _esp_interfaces[type]->lwip_netif;
    if(!interface) {
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

esp_err_t CustomNetif::add_chain(const interface_t type, input_chain_filter_fn filter, input_chain_process_fn process) {
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

err_t CustomNetif::traverse_input_chain(const interface_t type, struct pbuf *p, struct netif *inp) {
    if(!_interfaces[type]) {
        return ERR_OK;
    }
    for(uint8_t i = 0 ; i < MAXIMUM_CHAIN ; i++) {
        if(!_input_chain[type][i].filter && _input_chain[type][i].process) {
            if(TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN == _input_chain[type][i].process(type, p)) {
                return ERR_OK;
            }
        } else if(_input_chain[type][i].filter && _input_chain[type][i].filter(type, p)) {
            if(TYPE_CONSUME_PACKET_AND_EXIT_INPUT_CHAIN == _input_chain[type][i].process(type, p)) {
                return ERR_OK;
            }
        } else if(!_input_chain[type][i].filter && !_input_chain[type][i].process) {
            break;
        }
    }
    return _original_inputs[type](p, inp);
}

err_t CustomNetif::l3transmit(const interface_t type, struct pbuf *p, ip4_addr_t* nexthop) {
    struct netif* iface = _interfaces[type];
    ip4_addr_t route;
    err_t err;
    if(!iface) {
        return ERR_IF;
    }

    /* Simple Link local routing */
    if(nexthop) {
        route.addr = nexthop->addr;
    } else {
        if((IP4(p)->dest.addr & iface->netmask.u_addr.ip4.addr) == (iface->ip_addr.u_addr.ip4.addr & iface->netmask.u_addr.ip4.addr)) {
            // Link local addresses including link local broadcast will fall into this case.
            route.addr = IP4(p)->dest.addr;
        } else if (((((uint8_t*)&(IP4(p)->dest.addr))[0] & 0xf0) == 0xe0) ||
                   IP4(p)->dest.addr == 0xffffffff) {
            // Broadcast and multicast addresses will fall into this case.
            route.addr = IP4(p)->dest.addr;
        } else {
            // Default routing to gw.
            route.addr = iface->gw.u_addr.ip4.addr;
        }
    }

    // reset eth header.
    pbuf_header(p, -sizeof(struct eth_hdr));
    for(uint8_t tx = 0 ; tx < CONFIG_MAXIMUM_RETRANSMISSION ; tx++) {
        err = iface->output(iface, p, &route);
        if(err == ERR_OK) {
            return ERR_OK;
        }
    }
    return err;
}

err_t CustomNetif::transmit(const interface_t type, struct pbuf *p) {
    struct netif* iface = _interfaces[type];
    err_t err;
    if(!iface) {
        return ERR_IF;
    }
    memcpy(ETH(p)->src.addr, iface->hwaddr, NETIF_MAX_HWADDR_LEN);
    for(uint8_t tx = 0 ; tx < CONFIG_MAXIMUM_RETRANSMISSION ; tx++) {
        err = iface->linkoutput(iface, p);
        if(err == ERR_OK) {
            return ERR_OK;
        }
    }
    return err;
}
