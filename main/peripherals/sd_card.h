//
// Created by samuel on 18-8-22.
//

#ifndef APP_TEMPLATE_SD_CARD_H
#define APP_TEMPLATE_SD_CARD_H

void sd_card_init();

void sd_card_deinit();

void sd_card_test();

void sd_card_file_append(char *file_name, char *line);

void sd_card_create_file_incremental(char *base_file_name, char *base_file_extension, int iteration, char *file_name_out);

#endif //APP_TEMPLATE_SD_CARD_H
