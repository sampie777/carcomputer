//
// Created by samuel on 31-7-22.
//

#include <driver/i2c.h>
#include "mpu9250.h"
#include "../utils.h"
#include "display/display.h"


#define MPU9250_REGISTER_SELF_TEST_X_GYRO 0x00
#define MPU9250_REGISTER_SELF_TEST_Y_GYRO 0x01
#define MPU9250_REGISTER_SELF_TEST_Z_GYRO 0x02
#define MPU9250_REGISTER_SELF_TEST_X_ACCEL 0x0D
#define MPU9250_REGISTER_SELF_TEST_Y_ACCEL 0x0E
#define MPU9250_REGISTER_SELF_TEST_Z_ACCEL 0x0F
#define MPU9250_REGISTER_XG_OFFSET_H 0x13
#define MPU9250_REGISTER_XG_OFFSET_L 0x14
#define MPU9250_REGISTER_YG_OFFSET_H 0x15
#define MPU9250_REGISTER_YG_OFFSET_L 0x16
#define MPU9250_REGISTER_ZG_OFFSET_H 0x17
#define MPU9250_REGISTER_ZG_OFFSET_L 0x18
#define MPU9250_REGISTER_SMPLRT_DIV 0x19
#define MPU9250_REGISTER_CONFIG 0x1A
#define MPU9250_REGISTER_GYRO_CONFIG 0x1B
#define MPU9250_REGISTER_ACCEL_CONFIG 0x1C
#define MPU9250_REGISTER_ACCEL_CONFIG2 0x1D
#define MPU9250_REGISTER_LP_ACCEL_ODR 0x1E
#define MPU9250_REGISTER_WOM_THR 0x1F
#define MPU9250_REGISTER_FIFO_EN 0x23
#define MPU9250_REGISTER_I2C_MST_CTRL 0x24
#define MPU9250_REGISTER_INT_PIN_CFG 0x37

#define MPU9250_REGISTER_ACCEL_XOUT_H 0x3B
#define MPU9250_REGISTER_ACCEL_XOUT_L 0x3C
#define MPU9250_REGISTER_ACCEL_YOUT_H 0x3D
#define MPU9250_REGISTER_ACCEL_YOUT_L 0x3E
#define MPU9250_REGISTER_ACCEL_ZOUT_H 0x3F
#define MPU9250_REGISTER_ACCEL_ZOUT_L 0x40
#define MPU9250_REGISTER_TEMP_OUT_H 0x41
#define MPU9250_REGISTER_TEMP_OUT_L 0x42
#define MPU9250_REGISTER_GYRO_XOUT_H 0x43
#define MPU9250_REGISTER_GYRO_XOUT_L 0x44
#define MPU9250_REGISTER_GYRO_YOUT_H 0x45
#define MPU9250_REGISTER_GYRO_YOUT_L 0x46
#define MPU9250_REGISTER_GYRO_ZOUT_H 0x47
#define MPU9250_REGISTER_GYRO_ZOUT_L 0x48

#define MPU9250_REGISTER_USER_CTRL 0x6A
#define MPU9250_REGISTER_PWR_MGMT_1 0x6B
#define MPU9250_REGISTER_PWR_MGMT_2 0x6C
#define MPU9250_REGISTER_FIFO_COUNTH 0x72
#define MPU9250_REGISTER_FIFO_COUNTL 0x73
#define MPU9250_REGISTER_FIFO_R_W 0x74
#define MPU9250_REGISTER_WHO_AM_I 0x75

#define MPU9250_REGISTER_XA_OFFSET_H 0x77
#define MPU9250_REGISTER_XA_OFFSET_L 0x78
#define MPU9250_REGISTER_YA_OFFSET_H 0x7A
#define MPU9250_REGISTER_YA_OFFSET_L 0x7B
#define MPU9250_REGISTER_ZA_OFFSET_H 0x7D
#define MPU9250_REGISTER_ZA_OFFSET_L 0x7E

#define AK8963_REGISTER_WIA 0x00
#define AK8963_REGISTER_INFO 0x01
#define AK8963_REGISTER_ST1 0x02
#define AK8963_REGISTER_HXL 0x03
#define AK8963_REGISTER_HXH 0x04
#define AK8963_REGISTER_HYL 0x05
#define AK8963_REGISTER_HYH 0x06
#define AK8963_REGISTER_HZL 0x07
#define AK8963_REGISTER_HZH 0x08
#define AK8963_REGISTER_ST2 0x09
#define AK8963_REGISTER_CNTL 0x0A
#define AK8963_REGISTER_RSV 0x0B
#define AK8963_REGISTER_ASTC 0x0C
#define AK8963_REGISTER_TS1 0x0D
#define AK8963_REGISTER_TS2 0x0E
#define AK8963_REGISTER_I2CDIS 0x0F
#define AK8963_REGISTER_ASAX 0x10
#define AK8963_REGISTER_ASAY 0x11
#define AK8963_REGISTER_ASAZ 0x12

void request_register(uint8_t address, uint8_t reg) {
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(command, reg, true);
    i2c_master_stop(command);

    esp_err_t result = i2c_master_cmd_begin(MAIN_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
        printf("[mpu9250] I2C request register transmission failed: 0x%03x\n", result);
    }
    i2c_cmd_link_delete(command);
}

void mpu9250_set_register(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (MOTION_SENSOR_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(command, reg, true);
    i2c_master_write_byte(command, data, true);
    i2c_master_stop(command);

    esp_err_t result = i2c_master_cmd_begin(MAIN_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
        printf("[mpu9250] I2C set register transmission failed: 0x%03x\n", result);
    }
    i2c_cmd_link_delete(command);
}

int mpu9250_get_whois() {
    request_register(MOTION_SENSOR_I2C_ADDRESS, MPU9250_REGISTER_WHO_AM_I);

    uint8_t data = 0;
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (MOTION_SENSOR_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(command, &data, I2C_MASTER_NACK);

    i2c_master_stop(command);
    esp_err_t result = i2c_master_cmd_begin(MAIN_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
        printf("[mpu9250] I2C whois init transmission failed: 0x%03x\n", result);
        data = -1;
    }
    i2c_cmd_link_delete(command);

    return data;
}

void mpu9250_read_motion(State *state) {
    request_register(MOTION_SENSOR_I2C_ADDRESS, MPU9250_REGISTER_ACCEL_XOUT_H);

    uint8_t accel_data[6];
    uint8_t temperature_data[2];
    uint8_t gyro_data[6];

    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (MOTION_SENSOR_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(command, accel_data, sizeof(accel_data), I2C_MASTER_ACK);
    i2c_master_read(command, temperature_data, sizeof(temperature_data), I2C_MASTER_ACK);
    i2c_master_read(command, gyro_data, sizeof(gyro_data), I2C_MASTER_LAST_NACK);

    i2c_master_stop(command);
    esp_err_t result = i2c_master_cmd_begin(MAIN_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
        printf("[mpu9250] I2C motion read transmission failed: 0x%03x\n", result);
    }
    i2c_cmd_link_delete(command);

    state->motion.accel_x = (int16_t) ((accel_data[0] << 8) | accel_data[1]) / 32768.0 * 4;
    state->motion.accel_y = (int16_t) ((accel_data[2] << 8) | accel_data[3]) / 32768.0 * 4;
    state->motion.accel_z = (int16_t) ((accel_data[4] << 8) | accel_data[5]) / 32768.0 * 4;
    state->motion.gyro_x = (int16_t) ((gyro_data[0] << 8) | gyro_data[1]) / 32768.0 * 500;
    state->motion.gyro_y = (int16_t) ((gyro_data[2] << 8) | gyro_data[3]) / 32768.0 * 500;
    state->motion.gyro_z = (int16_t) ((gyro_data[4] << 8) | gyro_data[5]) / 32768.0 * 500;

    state->motion.temperature = (int16_t) ((temperature_data[0] << 8) | temperature_data[1]) * 0.15;
    state->motion.temperature = ((state->motion.temperature - MOTION_SENSOR_ROOM_TEMPERATURE_OFFSET) / MOTION_SENSOR_TEMPERATURE_SENSITIVITY) + 21.0;
}

void mpu9250_read_compass(State *state) {
    request_register(COMPASS_SENSOR_I2C_ADDRESS, AK8963_REGISTER_ST1);

    uint8_t status1;
    uint8_t data[6];
    uint8_t status2;

    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (COMPASS_SENSOR_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(command, &status1, I2C_MASTER_ACK);
    i2c_master_read(command, data, sizeof(data), I2C_MASTER_ACK);
    i2c_master_read_byte(command, &status2, I2C_MASTER_LAST_NACK);

    i2c_master_stop(command);
    esp_err_t result = i2c_master_cmd_begin(MAIN_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
        printf("[mpu9250] I2C compass read transmission failed: 0x%03x\n", result);
    }
    i2c_cmd_link_delete(command);

    // Check for Magnetic sensor overflow: data is not correct
    if (status2 & 0x08) {
        printf("[mpu9250] Magnetic sensor overflow exception\n");
        state->motion.compass_x = 0;
        state->motion.compass_y = 0;
        state->motion.compass_z = 0;
        return;
    }

    state->motion.compass_x = (int16_t) (data[0] | (data[1] << 8)) / 32768.0 * 4912;
    state->motion.compass_y = (int16_t) (data[2] | (data[3] << 8)) / 32768.0 * 4912;
    state->motion.compass_z = (int16_t) (data[4] | (data[5] << 8)) / 32768.0 * 4912;
}

void mpu9250_read(State *state) {
    static int64_t last_read_time = 0;
    if (esp_timer_get_time_ms() < last_read_time + MOTION_SENSOR_READ_INTERVAL_MS) {
        return;
    }
    last_read_time = esp_timer_get_time_ms();

    if (mpu9250_get_whois() == -1) {
        state->motion.connected = false;
        return;
    }
    state->motion.connected = true;

    mpu9250_read_motion(state);
    mpu9250_read_compass(state);
}

void mpu9250_init_compass() {
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    i2c_master_start(command);
    i2c_master_write_byte(command, (COMPASS_SENSOR_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

    i2c_master_write_byte(command, AK8963_REGISTER_CNTL, true);
    i2c_master_write_byte(command, 0x16, true); // Set to 16-bit and 100 Hz continuous mode

    i2c_master_stop(command);
    esp_err_t result = i2c_master_cmd_begin(MAIN_I2C_PORT, command, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
        printf("[mpu9250] I2C compass init transmission failed: 0x%03x\n", result);
    }
    i2c_cmd_link_delete(command);
}

void mpu9250_init_motion() {
    mpu9250_set_register(MPU9250_REGISTER_PWR_MGMT_1, 0x80);    // Reset chip
    delay_ms(100);
    mpu9250_set_register(MPU9250_REGISTER_PWR_MGMT_1, 0x00);    // Wake-up chip
    delay_ms(50);
    mpu9250_set_register(MPU9250_REGISTER_PWR_MGMT_1, 0x01);    // Auto select clock
    delay_ms(50);

    mpu9250_set_register(MPU9250_REGISTER_CONFIG, 0x03);            // Set bandwidth of gyro and temp to 41/42 Hz
    mpu9250_set_register(MPU9250_REGISTER_SMPLRT_DIV, 4);           // Set sample rate to 200 Hz
    mpu9250_set_register(MPU9250_REGISTER_GYRO_CONFIG, 0x01 << 3);  // Set sensitivity to +/-500 dps
    mpu9250_set_register(MPU9250_REGISTER_XG_OFFSET_H, ((int16_t) (GYRO_X_OFFSET * 32768 / 500.0 / -2.0)) >> 8);
    mpu9250_set_register(MPU9250_REGISTER_XG_OFFSET_L, ((int16_t) (GYRO_X_OFFSET * 32768 / 500.0 / -2.0)));
    mpu9250_set_register(MPU9250_REGISTER_YG_OFFSET_H, ((int16_t) (GYRO_Y_OFFSET * 32768 / 500.0 / -2.0)) >> 8);
    mpu9250_set_register(MPU9250_REGISTER_YG_OFFSET_L, ((int16_t) (GYRO_Y_OFFSET * 32768 / 500.0 / -2.0)));
    mpu9250_set_register(MPU9250_REGISTER_ZG_OFFSET_H, ((int16_t) (GYRO_Z_OFFSET * 32768 / 500.0 / -2.0)) >> 8);
    mpu9250_set_register(MPU9250_REGISTER_ZG_OFFSET_L, ((int16_t) (GYRO_Z_OFFSET * 32768 / 500.0 / -2.0)));
    mpu9250_set_register(MPU9250_REGISTER_ACCEL_CONFIG, 0x01 << 3); // Set sensitivity to +/- 4g
    mpu9250_set_register(MPU9250_REGISTER_ACCEL_CONFIG2, 0x01);     // Set bandwidth to 184 Hz
    mpu9250_set_register(MPU9250_REGISTER_INT_PIN_CFG, 0x02);       // Enable master/slave bypass
}

void mpu9250_init() {
    printf("[mpu9250] Initializing...\n");

    mpu9250_init_motion();
    mpu9250_init_compass();

    delay_ms(50);
    printf("[mpu9250] Init done\n");
}