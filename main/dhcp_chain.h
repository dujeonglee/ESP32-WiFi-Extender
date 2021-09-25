/**
 * @file dhcp_chain.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief DHCP chain
 * @version 0.1
 * @date 2020-02-09
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef DHCPCHAIN_H
#define DHCPCHAIN_H
#include "common.h"

bool dhcp_filter_ap(const interface_t type, struct pbuf *p);
pkt_fate_t dhcp_process_ap(const interface_t type, struct pbuf *p);
bool dhcp_filter_sta(const interface_t type, struct pbuf *p);
pkt_fate_t dhcp_process_sta(const interface_t type, struct pbuf *p);
#endif
