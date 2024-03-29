//
// Created by samuel on 21-7-22.
//

#ifndef APP_TEMPLATE_MCP2515_WRAPPER_H
#define APP_TEMPLATE_MCP2515_WRAPPER_H


#include "canbus.h"

#ifdef __cplusplus
extern "C" {
#endif

int mcp2515_init(bool listen_only);
int mcp2515_read_message(CanMessage *message);
int mcp2515_send_message(const CanMessage *message);
uint8_t mcp2515_get_mode();
uint8_t mcp2515_get_config3();

#ifdef __cplusplus
}
#endif

#endif //APP_TEMPLATE_MCP2515_WRAPPER_H
