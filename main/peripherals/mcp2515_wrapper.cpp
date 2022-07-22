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
    mcp2515->reset();
    mcp2515->setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515->setFilterMask(MCP2515::MASK0, false, 0x07ff);
    mcp2515->setFilter(MCP2515::RXF0, false, 385); // RPM filter
    mcp2515->setFilter(MCP2515::RXF1, false, 852); // Speed & brake filter
    mcp2515->setListenOnlyMode();

    return RESULT_OK;
}

int mcp2515_read_message(CanMessage *message) {
    can_frame frame = {};
    if (mcp2515->readMessage(&frame) != MCP2515::ERROR_OK){
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