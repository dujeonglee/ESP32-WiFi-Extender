/**
 * @file common.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief Common utility functions
 * @version 0.1
 * @date 2020-02-09
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef COMMON_H
#define COMMON_H

#include <string.h>

#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"

#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/prot/ethernet.h"
#include "lwip/prot/etharp.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/tcp.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/dhcp.h"

#include "tcpip_adapter.h"
#include "nvs_flash.h"
#include "nvs.h"

#define ETH(p)       (((struct eth_hdr*)(uint8_t*)p->payload))
#define ARP(p)       (((struct etharp_hdr*)((uint8_t*)p->payload + sizeof(struct eth_hdr))))
#define IP4(p)       (((struct ip_hdr*)((uint8_t*)p->payload + sizeof(struct eth_hdr))))
#define TCP4(p)      (((struct tcp_hdr*)((uint8_t*)p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr))))
#define UDP4(p)      (((struct udp_hdr*)((uint8_t*)p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr))))
#define DHCP(p)      (((struct dhcp_msg*)((uint8_t*)p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr))))

#endif
