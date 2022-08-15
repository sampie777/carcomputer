//
// Created by samuel on 21-7-22.
//

#ifndef APP_TEMPLATE_MCP2515_WRAPPER_H
#define APP_TEMPLATE_MCP2515_WRAPPER_H


#include "canbus.h"

#ifdef __cplusplus
extern "C" {
#endif

int mcp2515_init();
int mcp2515_read_message(CanMessage *message);
int mcp2515_get_mode();

#ifdef __cplusplus
}
#endif

#endif //APP_TEMPLATE_MCP2515_WRAPPER_H
