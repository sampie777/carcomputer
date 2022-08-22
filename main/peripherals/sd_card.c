//
// Created by samuel on 18-8-22.
//

#include <driver/sdspi_host.h>
#include <sdmmc_cmd.h>
#include <esp_vfs_fat.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "sd_card.h"
#include "../config.h"

static const char *TAG = "SD";
#define MOUNT_POINT "/sdcard"
const char mount_point[] = MOUNT_POINT;

sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *card;

void sd_card_test() {
    // Source: https://github.com/espressif/esp-idf/blob/v4.4.1/examples/storage/sd_card/sdspi/main/sd_card_example_main.c

    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";

    ESP_LOGI(TAG, "Opening file %s", file_hello);
    FILE *f = fopen(file_hello, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello %s!\n", card->cid.name);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    const char *file_foo = MOUNT_POINT"/foo.txt";

    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file %s", file_foo);
    f = fopen(file_foo, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    // Read a line from file
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);

    // Strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);
}

void sd_card_file_append(char *file_name, char *line) {
    char path[64];
    sprintf(path, "%s/%s", MOUNT_POINT, file_name);

    FILE *file = fopen(path, "a");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return;
    }
    fprintf(file, "%s", line);
    fclose(file);
}

/**
 * Create a file with given name and extension. If filename already exists, increment new filename with a number until unique.
 * The new filename will be stored in the file_name parameter.
  * @param base_file_name
  * @param iteration        Iteration number to start with
  * @param file_name_out    The new file name will be stored in here (size: 32)
  */
void sd_card_create_file_incremental(char *base_file_name, char *base_file_extension, int iteration, char *file_name_out) {
    char new_file_name[32];
    sprintf(new_file_name, "%s-%d.%s", base_file_name, iteration, base_file_extension);

    char path[128];
    sprintf(path, "%s/%s", MOUNT_POINT, new_file_name);

    struct stat st;
    if (stat(path, &st) == 0) {
        printf("[SD] File already exists: %s\n", path);
        sd_card_create_file_incremental(base_file_name, base_file_extension, iteration + 1, file_name_out);
        return;
    }

    memcpy(file_name_out, new_file_name, 32);
}

void sd_card_deinit() {
    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");
}

void sd_card_init() {
    printf("[SD] Initializing...\n");

    host.max_freq_khz = 15000;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = SD_CHIP_SELECT_PIN;
    slot.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    esp_err_t ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);

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
           card->cid.mfg_id,
           card->cid.oem_id,
           card->cid.name,
           card->cid.revision,
           card->cid.serial,
           card->cid.date,
           card->csd.csd_ver,
           card->csd.mmc_ver,
           card->csd.capacity,
           card->csd.sector_size,
           card->csd.read_block_len,
           card->csd.card_command_class,
           card->csd.tr_speed
    );
    printf("[SD] Init done\n");
}
