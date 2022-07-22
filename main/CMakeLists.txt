# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c utils.c control.c connectivity/wifi.c connectivity/bluetooth.c connectivity/spi.c lib/esp32-mcp2515/mcp2515.cpp peripherals/display.c peripherals/canbus.c peripherals/gas_pedal.c peripherals/mcp2515_wrapper.cpp ")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
