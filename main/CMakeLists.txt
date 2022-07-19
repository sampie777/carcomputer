# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c peripherals/display.c utils.c connectivity/wifi.c connectivity/bluetooth.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
