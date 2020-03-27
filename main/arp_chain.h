/**
 * @file arp_chain.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief Proxy ARP
 * @version 0.1
 * @date 2020-02-09
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef ARPCHAIN_H
#define ARPCHAIN_H
#include "common.h"

bool arp_filter_ap(const tcpip_adapter_if_t type, struct pbuf *p);
pkt_fate_t arp_process_ap(const tcpip_adapter_if_t type, struct pbuf *p);
bool arp_filter_sta(const tcpip_adapter_if_t type, struct pbuf *p);
pkt_fate_t arp_process_sta(const tcpip_adapter_if_t type, struct pbuf *p);
#endif
