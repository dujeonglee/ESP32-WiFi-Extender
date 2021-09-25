/**
 * @file route_chain.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief Route chain
 * @version 0.1
 * @date 2020-02-10
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef ROUTECHAIN_H
#define ROUTECHAIN_H
#include "common.h"

bool route_filter_ap(const interface_t type, struct pbuf *p);
pkt_fate_t route_process_ap(const interface_t type, struct pbuf *p);
bool route_filter_sta(const interface_t type, struct pbuf *p);
pkt_fate_t route_process_sta(const interface_t type, struct pbuf *p);
#endif
