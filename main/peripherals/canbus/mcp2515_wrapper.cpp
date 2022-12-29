//
// Created by samuel on 21-7-22.
//

#include <cstring>
#include <hal/gpio_types.h>
#include "../../config.h"
#include "mcp2515_wrapper.h"

#include "../../lib/esp32-mcp2515/mcp2515.h"
#include "../../connectivity/spi.h"
#include "../../return_codes.h"
#include "can_definitions.h"

static MCP2515 *mcp2515;

int mcp2515_init(bool listen_only) {
    static spi_device_handle_t spi_handle;
    spi_register_device(&spi_handle, CANBUS_CHIP_SELECT_PIN);

    mcp2515 = new MCP2515(&spi_handle);
    if (mcp2515->reset() != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to reset\n");
    if (mcp2515->setBitrate(CAN_500KBPS, MCP_8MHZ) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set bitrate\n");

    if (listen_only) {
        if (mcp2515->setListenOnlyMode() != MCP2515::ERROR_OK)
            printf("[mcp2515] Failed to set setListenOnlyMode\n");
    } else {
        if (mcp2515->setNormalMode() != MCP2515::ERROR_OK)
            printf("[mcp2515] Failed to set setNormalMode\n");
    }

    // High priority buffer
    if (mcp2515->setFilterMask(MCP2515::MASK0, false, 0x07ff) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter mask 0\n");
    if (mcp2515->setFilter(MCP2515::RXF0, false, CAN_ID_SPEED_AND_BRAKE) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 0\n");
    if (mcp2515->setFilter(MCP2515::RXF1, false, CAN_ID_RPM) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 1\n");

    // Low priority buffer (which is also used as rollover)
    if (mcp2515->setFilterMask(MCP2515::MASK1, false, 0x07ff) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter mask 1\n");
    if (mcp2515->setFilter(MCP2515::RXF2, false, CAN_ID_IGNITION) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 2\n");
    if (mcp2515->setFilter(MCP2515::RXF3, false, CAN_ID_DOOR_LOCKS) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 3\n");
    if (mcp2515->setFilter(MCP2515::RXF4, false, CAN_ID_ODOMETER) != MCP2515::ERROR_OK)
        printf("[mcp2515] Failed to set filter 4\n");

    if (listen_only) {
        if (mcp2515->setListenOnlyMode() != MCP2515::ERROR_OK)
            printf("[mcp2515] Failed to set setListenOnlyMode\n");
    } else {
        if (mcp2515->setNormalMode() != MCP2515::ERROR_OK)
            printf("[mcp2515] Failed to set setNormalMode\n");
    }

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

int mcp2515_send_message(const CanMessage *message) {
    struct can_frame frame = {
            .can_id = message->id,
            .can_dlc = message->length,
            .data = {0, 0, 0, 0, 0, 0, 0, 0}
    };
    memcpy(frame.data, message->data, message->length);

    if (mcp2515->sendMessage(&frame) != MCP2515::ERROR_OK) {
        return RESULT_FAILED;
    }
    return RESULT_OK;
}

uint8_t mcp2515_get_mode() {
    return mcp2515->getMode();
}

uint8_t mcp2515_get_config3() {
    return mcp2515->getConfig3();
}