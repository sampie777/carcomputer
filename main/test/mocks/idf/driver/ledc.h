//
// Created by Samuel-Anton Jansen on 2025/11/06.
//

#ifndef CARCOMPUTER_LEDC_H
#define CARCOMPUTER_LEDC_H

#include <stdint.h>
#include <driver/gpio.h>
#include <hal/ledc_types.h>

typedef enum {
    LEDC_AUTO_CLK = 0,                              /*!< LEDC source clock will be automatically selected based on the giving resolution and duty parameter when init the timer*/
    LEDC_USE_APB_CLK = 1,             /*!< Select APB as the source clock */
    // LEDC_USE_RC_FAST_CLK = SOC_MOD_CLK_RC_FAST,     /*!< Select RC_FAST as the source clock */
    // LEDC_USE_REF_TICK = SOC_MOD_CLK_REF_TICK,       /*!< Select REF_TICK as the source clock */

    // LEDC_USE_RTC8M_CLK __attribute__((deprecated("please use 'LEDC_USE_RC_FAST_CLK' instead"))) = LEDC_USE_RC_FAST_CLK,   /*!< Alias of 'LEDC_USE_RC_FAST_CLK' */
} soc_periph_ledc_clk_src_legacy_t;

typedef struct {
    ledc_mode_t speed_mode;                /*!< LEDC speed speed_mode, high-speed mode (only exists on esp32) or low-speed mode */
    ledc_timer_bit_t duty_resolution;      /*!< LEDC channel duty resolution */
    ledc_timer_t  timer_num;               /*!< The timer source of channel (0 - LEDC_TIMER_MAX-1) */
    uint32_t freq_hz;                      /*!< LEDC timer frequency (Hz) */
    ledc_clk_cfg_t clk_cfg;                /*!< Configure LEDC source clock from ledc_clk_cfg_t.
                                                Note that LEDC_USE_RC_FAST_CLK and LEDC_USE_XTAL_CLK are
                                                non-timer-specific clock sources. You can not have one LEDC timer uses
                                                RC_FAST_CLK as the clock source and have another LEDC timer uses XTAL_CLK
                                                as its clock source. All chips except esp32 and esp32s2 do not have
                                                timer-specific clock sources, which means clock source for all timers
                                                must be the same one. */
    bool deconfigure;                      /*!< Set this field to de-configure a LEDC timer which has been configured before
                                                Note that it will not check whether the timer wants to be de-configured
                                                is binded to any channel. Also, the timer has to be paused first before
                                                it can be de-configured.
                                                When this field is set, duty_resolution, freq_hz, clk_cfg fields are ignored. */
} ledc_timer_config_t;

typedef enum {
    LEDC_SLEEP_MODE_NO_ALIVE_NO_PD = 0,  /*!< The default mode: no LEDC output, and no power off the LEDC power domain. */
    LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PD,   /*!< The low-power-consumption mode: no LEDC output, and allow to power off the LEDC power domain.
                                              This can save power, but at the expense of more RAM being consumed to save register context.
                                              This option is only available on targets that support TOP domain to be powered down. */
    LEDC_SLEEP_MODE_KEEP_ALIVE,          /*!< The high-power-consumption mode: keep LEDC output when the system enters Light-sleep. */
    LEDC_SLEEP_MODE_INVALID,             /*!< Invalid LEDC sleep mode strategy */
} ledc_sleep_mode_t;

typedef struct {
    int gpio_num;                   /*!< the LEDC output gpio_num, if you want to use gpio16, gpio_num = 16 */
    ledc_mode_t speed_mode;         /*!< LEDC speed speed_mode, high-speed mode (only exists on esp32) or low-speed mode */
    ledc_channel_t channel;         /*!< LEDC channel (0 - LEDC_CHANNEL_MAX-1) */
    ledc_intr_type_t intr_type;     /*!< configure interrupt, Fade interrupt enable  or Fade interrupt disable */
    ledc_timer_t timer_sel;         /*!< Select the timer source of channel (0 - LEDC_TIMER_MAX-1) */
    uint32_t duty;                  /*!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)] */
    int hpoint;                     /*!< LEDC channel hpoint value, the range is [0, (2**duty_resolution)-1] */
    ledc_sleep_mode_t sleep_mode;   /*!< choose the desired behavior for the LEDC channel in Light-sleep */
    struct {
        unsigned int output_invert: 1;/*!< Enable (1) or disable (0) gpio output invert */
    } flags;                        /*!< LEDC flags */

} ledc_channel_config_t;

esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty);
esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel);
esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf);
esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf);

#endif //CARCOMPUTER_LEDC_H