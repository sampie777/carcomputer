//
// Created by samuel on 2025/02/09.
//

#include "adc.h"
#include "../utils.h"

adc_oneshot_unit_handle_t adc1_handle;

double adc_average_read_channel(adc_channel_t channel, int sample_count) {
    double total = 0;
    sample_count = max(1, sample_count);
    int valid_samples = 0;
    for (int i = 0; i < sample_count; i++) {
        int result;
        esp_err_t error = adc_oneshot_read(adc1_handle, channel, &result);
        if (error != ESP_OK) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(error);
            break;
        }

        total += result;
        valid_samples++;

        // Add a bit of delay (10 clock cycles) to get a new sample
        for (volatile int j = 0; j < 10; j++) {
        }
    }
    if (valid_samples == 0) {
        return -1.0;
    }
    return total / valid_samples;
}

void adc_oneshot_init_channel(adc_channel_t channel) {
    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_RESOLUTION,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channel, &adc_config));
}

void adc_oneshot_init() {
    adc_oneshot_unit_init_cfg_t init_adc_config0 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_adc_config0, &adc1_handle));
}
