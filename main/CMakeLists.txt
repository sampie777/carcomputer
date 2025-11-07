option(RUN_TESTS "Build and run component tests" OFF)

if (RUN_TESTS)
    cmake_minimum_required(VERSION 3.16)
    project(tests C)

    set(CMAKE_C_STANDARD 11)

    add_executable(tests test.c test/utils/csv.c test/utils/bmp.c test/test_cruisecontrol.c test/mocks/idf/esp_timer.c utils.c math.c test/mocks/carcomputer/peripherals/adc.c test/mocks/carcomputer/peripherals/gas_pedal.c control/cruise_control.c control/control.c test/mocks/carcomputer/peripherals/buttons.c test/mocks/carcomputer/peripherals/canbus/canbus.c test/mocks/carcomputer/control/buttons.c test/mocks/carcomputer/peripherals/mpu9250.c test/mocks/idf/driver/gpio.c )

    # Add the directory containing your mock headers
    target_include_directories(tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks/idf  # Assuming mocks/ is relative to the test CMakeLists.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks/carcomputer  # Assuming mocks/ is relative to the test CMakeLists.txt
            # Add other necessary include directories for your project
    )

else ()
    # Edit following two lines to set component requirements (see docs)
    set(COMPONENT_REQUIRES)
    set(COMPONENT_PRIV_REQUIRES)

    set(COMPONENT_SRCS "main.c tasks/task_primary.c tasks/task_secondary.c utils.c control/control.c control/cruise_control.c connectivity/i2c.c connectivity/spi.c lib/esp32-mcp2515/mcp2515.cpp peripherals/canbus/canbus.c peripherals/gas_pedal.c peripherals/canbus/mcp2515_wrapper.cpp peripherals/buttons.c peripherals/display/display.c peripherals/display/sh1106.c peripherals/display/display_screens.c peripherals/mpu9250.c control/buttons.c peripherals/sd_card.c control/data_logger.c peripherals/gpsgsm/utils.c peripherals/gpsgsm/gpsgsm.c peripherals/gpsgsm/a9g.c peripherals/adc.c math.c")
    set(COMPONENT_ADD_INCLUDEDIRS "")

    register_component()
endif ()