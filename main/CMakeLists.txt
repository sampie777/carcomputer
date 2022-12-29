# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS "main.c utils.c control/control.c control/cruise_control.c control/data_logger.c control/security.c connectivity/wifi.c connectivity/bluetooth.c connectivity/spi.c connectivity/i2c.c lib/esp32-mcp2515/mcp2515.cpp peripherals/display/display.c peripherals/canbus/canbus.c peripherals/gas_pedal.c peripherals/canbus/mcp2515_wrapper.cpp peripherals/display/sh1106.c peripherals/display/display_screens.c peripherals/buttons.c peripherals/mpu9250.c peripherals/sd_card.c peripherals/gpsgsm/gpsgsm.c peripherals/gpsgsm/utils.c backend/server.c backend/auth.c backend/utils.c backend/json.c utils/nvs.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
