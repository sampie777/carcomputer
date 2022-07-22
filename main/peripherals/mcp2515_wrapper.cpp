//
// Created by samuel on 21-7-22.
//

#include <cstring>
#include "../config.h"
#include "mcp2515_wrapper.h"

#include "../lib/esp32-mcp2515/mcp2515.h"
#include "../connectivity/spi.h"
#include "../return_codes.h"

static MCP2515 *mcp2515;

int mcp2515_init() {
    spi_device_handle_t spi_handle;
    spi_register_device(&spi_handle, CANBUS_CHIP_SELECT_PIN);

    mcp2515 = new MCP2515(&spi_handle);
    if (mcp2515->reset() != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to reset\n");
    if (mcp2515->setBitrate(CAN_500KBPS, MCP_8MHZ) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set bitrate\n");
    if (mcp2515->setFilterMask(MCP2515::MASK0, false, 0x07ff) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter mask\n");
    if (mcp2515->setFilter(MCP2515::RXF0, false, 385) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 0\n"); // RPM filter
    if (mcp2515->setFilter(MCP2515::RXF1, false, 852) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 1\n"); // Speed & brake filter
    if (mcp2515->setListenOnlyMode() != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set listen only mode\n");

    return RESULT_OK;
}

int mcp2515_read_message(CanMessage *message) {
    can_frame frame = {};
    if (mcp2515->readMessage(&frame) != MCP2515::ERROR_OK) {
        return RESULT_EMPTY;
    }
    message->id = frame.can_id;
    message->length = frame.can_dlc;
    memcpy(message->data, frame.data, 8);
    return RESULT_OK;
}

int mcp2515_get_status() {
    return mcp2515->getStatus();
}