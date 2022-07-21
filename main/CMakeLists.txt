# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c utils.c control.c connectivity/wifi.c connectivity/bluetooth.c peripherals/display.c peripherals/canbus.c peripherals/gas_pedal.c ")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
