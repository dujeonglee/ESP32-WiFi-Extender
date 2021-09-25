/**
 * @file bcmc_chain.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief Broadcast Multicast chain
 * @version 0.1
 * @date 2020-02-14
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef BCMCCHAIN_H
#define BCMCCHAIN_H
#include "common.h"

bool bcmc_filter(const interface_t type, struct pbuf *p);
pkt_fate_t bcmc_process_ap(const interface_t type, struct pbuf *p);
pkt_fate_t bcmc_process_sta(const interface_t type, struct pbuf *p);
#endif
