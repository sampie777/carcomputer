//
// Created by samuel on 2025/02/09.
//

#ifndef CARCOMPUTER_ADC_H
#define CARCOMPUTER_ADC_H

#include <esp_adc/adc_oneshot.h>

void adc_oneshot_init();
void adc_oneshot_init_channel(adc_channel_t channel_pin);
double adc_average_read_channel(adc_channel_t channel, int sample_count);

#endif //CARCOMPUTER_ADC_H
