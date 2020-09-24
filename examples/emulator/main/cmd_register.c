/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "argtable3/argtable3.h"
#include "emulator.h"

static void register_read();
static void register_write();
static void register_reset();
static void register_profile();
static void register_display_profiles();
static void register_reset_pairings();
static void register_auth();
static void register_reset_wifi_credentials();
static void register_reboot_accessory();

void register_system()
{
    register_auth();
    register_profile();
    register_display_profiles();
    register_reboot_accessory();
    register_reset_wifi_credentials();
    register_reset_pairings();
    register_reset();
    register_read();
    register_write();
}

/* Reading from characteristic sequence */
static int read_characteristic(int argc, char** argv)
{
    if(argc == 1) {
        emulator_read_chars(0);
    } else if(argc == 2) {
        emulator_read_chars(1, atoi(argv[1]));
    }  else if(argc == 3) {
        emulator_read_chars(2, atoi(argv[1]),atoi(argv[2]));
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/* Register profile switch sequence */
static int switch_profile(int argc, char** argv)
{
    if(argc == 2) {
        printf("Changing profile. Rebooting.\n");
        emulator_switch_profile(atoi(argv[1]));
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_read()
{
    const esp_console_cmd_t cmd = {
        .command = "read-char",
        .help = "Read the values of the charachteristics.\n Usage: read-char <aid> <iid>",
        .hint = NULL,
        .func = &read_characteristic,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Writing to characteristic sequence */
static int write_characteristic(int argc, char** argv)
{
    if(argc == 4) {
        printf("Characteristic write.\n");
        emulator_write_char(atoi(argv[1]),atoi(argv[2]),argv[3]);
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_write()
{
    const esp_console_cmd_t cmd = {
        .command = "write-char",
        .help = "Write values to the charachteristics.\n Usage: write-char <aid> <iid> <value>",
        .hint = NULL,
        .func = &write_characteristic,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Factory reset sequence */
static int reset(int argc, char** argv)
{
    if(argc == 1) {
        printf("Factory Resetting. Rebooting.\n");
        emulator_reset_to_factory();
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_reset()
{
    const esp_console_cmd_t cmd = {
        .command = "reset",
        .help = "Factory reset the device",
        .hint = NULL,
        .func = &reset,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Reset pairings sequence */
static int reset_pairings(int argc, char** argv)
{
    if(argc == 1) {
        printf("Resetting the pairings. Rebooting.\n");
        emulator_reset_pairings();
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_reset_pairings()
{
    const esp_console_cmd_t cmd = {
        .command = "reset-pairings",
        .help = "Reset the pairings of the device",
        .hint = NULL,
        .func = &reset_pairings,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Reset network credentials sequence */
static int reset_wifi_credentials(int argc, char** argv)
{
    if(argc == 1) {
        printf("Resetting the network credentials. Rebooting\n");
        emulator_reset_wifi_credentials();
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_reset_wifi_credentials()
{
    const esp_console_cmd_t cmd = {
        .command = "reset-network",
        .help = "Reset the wifi credentials.",
        .hint = NULL,
        .func = &reset_wifi_credentials,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Reboot accessory sequence */
static int reboot_accessory(int argc, char** argv)
{
    if(argc == 1) {
        printf("Rebooting the accessory.\n");
        emulator_reboot_accessory();
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_reboot_accessory()
{
    const esp_console_cmd_t cmd = {
        .command = "reboot",
        .help = "Reboot the accessory.",
        .hint = NULL,
        .func = &reboot_accessory,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}


static void register_profile()
{
    const esp_console_cmd_t cmd = {
        .command = "profile",
        .help = "Switch the profile of the device.\n Usage: profile <id>",
        .hint = NULL,
        .func = &switch_profile,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Display the profiles sequence */
static int display_profile(int argc, char** argv)
{
    if(argc == 1) {
        emulator_show_profiles();
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_display_profiles()
{
    const esp_console_cmd_t cmd = {
        .command = "profile-list",
        .help = "Check the available profiles.",
        .hint = NULL,
        .func = &display_profile,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/* Select the auth options */
static int select_auth(int argc, char** argv)
{
    if(argc == 2) {
        printf("Changing the auth option. Rebooting\n");
        emulator_set_auth(atoi(argv[1]));
    } else {
        printf("Invalid Usage.");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void register_auth()
{
    const esp_console_cmd_t cmd = {
        .command = "auth",
        .help = "Select the auth option. 0. None 1. Hardware 2. Software\n Usage: auth <id>",
        .hint = NULL,
        .func = &select_auth,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
