# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS "main.c tasks/task_primary.c tasks/task_secondary.c utils.c control/control.c control/cruise_control.c connectivity/i2c.c connectivity/spi.c lib/esp32-mcp2515/mcp2515.cpp peripherals/canbus/canbus.c peripherals/gas_pedal.c peripherals/canbus/mcp2515_wrapper.cpp peripherals/buttons.c peripherals/led.c peripherals/display/display.c peripherals/display/sh1106.c peripherals/display/display_screens.c peripherals/mpu9250.c control/buttons.c peripherals/sd_card.c control/data_logger.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
