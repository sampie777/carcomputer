# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c utils.c control/control.c control/cruise_control.c connectivity/wifi.c connectivity/bluetooth.c connectivity/spi.c connectivity/i2c.c lib/esp32-mcp2515/mcp2515.cpp peripherals/display/display.c peripherals/canbus.c peripherals/gas_pedal.c peripherals/mcp2515_wrapper.cpp peripherals/display/sh1106.c peripherals/buttons.c peripherals/mpu9250.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
