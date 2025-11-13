//
// Created by Samuel-Anton Jansen on 2025/11/13.
//

#ifndef CARCOMPUTER_UART_H
#define CARCOMPUTER_UART_H
#include <stdint.h>
#include <esp_err.h>
#include <freertos/queue.h>


#define UART_PIN_NO_CHANGE      (-1)

/**
 * @brief UART port number, can be UART_NUM_0 ~ (UART_NUM_MAX -1).
 */
typedef enum {
    UART_NUM_0,                         /*!< UART port 0 */
    UART_NUM_1,                         /*!< UART port 1 */
    UART_NUM_2,                         /*!< UART port 2 */
    UART_NUM_3,                         /*!< UART port 3 */
    UART_NUM_4,                         /*!< UART port 4 */
    LP_UART_NUM_0,                      /*!< LP UART port 0 */
    UART_NUM_MAX,                       /*!< UART port max */
} uart_port_t;

/**
 * @brief UART mode selection
 */
typedef enum {
    UART_MODE_UART = 0x00,                      /*!< mode: regular UART mode*/
    UART_MODE_RS485_HALF_DUPLEX = 0x01,         /*!< mode: half duplex RS485 UART mode control by RTS pin */
    UART_MODE_IRDA = 0x02,                      /*!< mode: IRDA  UART mode*/
    UART_MODE_RS485_COLLISION_DETECT = 0x03,    /*!< mode: RS485 collision detection UART mode (used for test purposes)*/
    UART_MODE_RS485_APP_CTRL = 0x04,            /*!< mode: application control RS485 UART mode (used for test purposes)*/
} uart_mode_t;

/**
 * @brief UART word length constants
 */
typedef enum {
    UART_DATA_5_BITS   = 0x0,    /*!< word length: 5bits*/
    UART_DATA_6_BITS   = 0x1,    /*!< word length: 6bits*/
    UART_DATA_7_BITS   = 0x2,    /*!< word length: 7bits*/
    UART_DATA_8_BITS   = 0x3,    /*!< word length: 8bits*/
    UART_DATA_BITS_MAX = 0x4,
} uart_word_length_t;

/**
 * @brief UART stop bits number
 */
typedef enum {
    UART_STOP_BITS_1   = 0x1,  /*!< stop bit: 1bit*/
    UART_STOP_BITS_1_5 = 0x2,  /*!< stop bit: 1.5bits*/
    UART_STOP_BITS_2   = 0x3,  /*!< stop bit: 2bits*/
    UART_STOP_BITS_MAX = 0x4,
} uart_stop_bits_t;

/**
 * @brief UART parity constants
 */
typedef enum {
    UART_PARITY_DISABLE  = 0x0,  /*!< Disable UART parity*/
    UART_PARITY_EVEN     = 0x2,  /*!< Enable UART even parity*/
    UART_PARITY_ODD      = 0x3   /*!< Enable UART odd parity*/
} uart_parity_t;

/**
 * @brief UART hardware flow control modes
 */
typedef enum {
    UART_HW_FLOWCTRL_DISABLE = 0x0,   /*!< disable hardware flow control*/
    UART_HW_FLOWCTRL_RTS     = 0x1,   /*!< enable RX hardware flow control (rts)*/
    UART_HW_FLOWCTRL_CTS     = 0x2,   /*!< enable TX hardware flow control (cts)*/
    UART_HW_FLOWCTRL_CTS_RTS = 0x3,   /*!< enable hardware flow control*/
    UART_HW_FLOWCTRL_MAX     = 0x4,
} uart_hw_flowcontrol_t;

/**
 * @brief UART signal bit map
 */
typedef enum {
    UART_SIGNAL_INV_DISABLE  =  0,            /*!< Disable UART signal inverse*/
    UART_SIGNAL_IRDA_TX_INV  = (0x1 << 0),    /*!< inverse the UART irda_tx signal*/
    UART_SIGNAL_IRDA_RX_INV  = (0x1 << 1),    /*!< inverse the UART irda_rx signal*/
    UART_SIGNAL_RXD_INV      = (0x1 << 2),    /*!< inverse the UART rxd signal*/
    UART_SIGNAL_CTS_INV      = (0x1 << 3),    /*!< inverse the UART cts signal*/
    UART_SIGNAL_DSR_INV      = (0x1 << 4),    /*!< inverse the UART dsr signal*/
    UART_SIGNAL_TXD_INV      = (0x1 << 5),    /*!< inverse the UART txd signal*/
    UART_SIGNAL_RTS_INV      = (0x1 << 6),    /*!< inverse the UART rts signal*/
    UART_SIGNAL_DTR_INV      = (0x1 << 7),    /*!< inverse the UART dtr signal*/
} uart_signal_inv_t;

typedef struct {
    int baud_rate;                      /*!< UART baud rate
                                             Note that the actual baud rate set could have a slight deviation from the user-configured value due to rounding error*/
    uart_word_length_t data_bits;       /*!< UART byte size*/
    uart_parity_t parity;               /*!< UART parity mode*/
    uart_stop_bits_t stop_bits;         /*!< UART stop bits*/
    uart_hw_flowcontrol_t flow_ctrl;    /*!< UART HW flow control mode (cts/rts)*/
    uint8_t rx_flow_ctrl_thresh;        /*!< UART HW RTS threshold*/
    struct {
        uint32_t allow_pd: 1;               /*!< If set, driver allows the power domain to be powered off when system enters sleep mode.
                                                 This can save power, but at the expense of more RAM being consumed to save register context. */
        uint32_t backup_before_sleep: 1;    /*!< @deprecated, same meaning as allow_pd */
    } flags;                                /*!< Configuration flags */
} uart_config_t;


typedef enum {
    UART_DATA,              /*!< Triggered when the receiver either takes longer than rx_timeout_thresh
                                 to receive a byte, or when more data is received than what rxfifo_full_thresh
                                 specifies*/
    UART_BREAK,             /*!< Triggered when the receiver detects a NULL character*/
    UART_BUFFER_FULL,       /*!< Triggered when RX ring buffer is full*/
    UART_FIFO_OVF,          /*!< Triggered when the received data exceeds the capacity of the RX FIFO*/
    UART_FRAME_ERR,         /*!< Triggered when the receiver detects a data frame error*/
    UART_PARITY_ERR,        /*!< Triggered when a parity error is detected in the received data*/
    UART_DATA_BREAK,        /*!< Internal event triggered to signal a break afte data transmission*/
    UART_PATTERN_DET,       /*!< Triggered when a specified pattern  is detected in the incoming data*/
    UART_WAKEUP,            /*!< Triggered when a wakeup signal is detected*/
    UART_EVENT_MAX,         /*!< Maximum index for UART events*/
} uart_event_type_t;

esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);
esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num);
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int event_queue_size, QueueHandle_t *uart_queue, int intr_alloc_flags);

#endif //CARCOMPUTER_UART_H