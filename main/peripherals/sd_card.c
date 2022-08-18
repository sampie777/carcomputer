//
// Created by samuel on 18-8-22.
//

#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
#include <sdmmc_cmd.h>
#include "sd_card.h"
#include "../config.h"


sdmmc_host_t host = SDSPI_HOST_DEFAULT();

void sd_card_init() {
    printf("[SD] Initializing...\n");
    sdspi_device_config_t config = SDSPI_DEVICE_CONFIG_DEFAULT();
    config.gpio_cs = SD_CHIP_SELECT_PIN;
    sdspi_dev_handle_t spi_handle;
    sdspi_host_init_device(&config, &spi_handle);

    host.slot = spi_handle;
    sdmmc_card_t card_info;
    sdmmc_card_init(&host, &card_info);
    printf("[SD] Init done: \n"
           "\t-- Card IDentification --\n"
           "\tmanufacturer identification number = %d\n"
           "\tOEM/product identification number = %d\n"
           "\tproduct name (MMC v1 has the longest) = %s\n"
           "\tproduct revision = %d\n"
           "\tproduct serial number = %d\n"
           "\tmanufacturing date = %d\n"
           "\t-- Card Specific Data --\n"
           "\tCSD structure format = %d\n"
           "\tMMC version (for CID format) = %d\n"
           "\ttotal number of sectors = %d\n"
           "\tsector size in bytes = %d\n"
           "\tblock length for reads = %d\n"
           "\tCard Command Class for SD = %d\n"
           "\tMax transfer speed = %d\n",
           card_info.cid.mfg_id,
           card_info.cid.oem_id,
           card_info.cid.name,
           card_info.cid.revision,
           card_info.cid.serial,
           card_info.cid.date,
           card_info.csd.csd_ver,
           card_info.csd.mmc_ver,
           card_info.csd.capacity,
           card_info.csd.sector_size,
           card_info.csd.read_block_len,
           card_info.csd.card_command_class,
           card_info.csd.tr_speed
    );
}