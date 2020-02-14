/**
 * @file debug_chain.h
 * @author Dujeong Lee (dujeong.lee82@gmail.com)
 * @brief debug chain
 * @version 0.1
 * @date 2020-02-09
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef DEBUGCHAIN_H
#define DEBUGCHAIN_H
#include "common.h"
pkt_fate_t debug_process(const tcpip_adapter_if_t type, struct pbuf *p);
#endif
