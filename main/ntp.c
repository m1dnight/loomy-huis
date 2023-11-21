#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// own code
#include "defines.h"

#define TAG "ntp"

/// @brief returns a time_t that holds the current time on the esp
time_t get_current_time()
{
    time_t now = 0;
    time(&now);
    return now;
}

//------------------------------------------------------------------------------
// time printing functions
//------------------------------------------------------------------------------

/// @brief prints out the given time and message to the console
void print_time_and_message(time_t time, char *message)
{
    setenv("TZ", TIMEZONE, 1);
    tzset();

    char       buffer[50];
    struct tm *timeinfo = localtime(&time);

    strftime(buffer, sizeof(buffer), "%c", timeinfo);
    ESP_LOGI(TAG, "%s: %s\n", message, buffer);

    char timestamp[64];

    strftime(timestamp, sizeof(timestamp), "%y%m%H%M%SW", timeinfo);
    ESP_LOGI(TAG, "%s: %s\n", message, timestamp);
}

/// @brief prints the given time and message to the console
void print_current_time_with_message(char *message)
{
    time_t now = 0;
    time(&now);
    print_time_and_message(now, message);
}

/// @brief prints the current time to the console
void print_current_time()
{
    time_t now = 0;
    time(&now);
    print_time_and_message(now, "current time");
}

//------------------------------------------------------------------------------
// ntp service
//------------------------------------------------------------------------------

/// @brief event handler when the time is updated
void on_got_time(struct timeval *tv)
{
    ESP_LOGI(TAG, "time value updated");

    time_t now = 0;
    time(&now);
    print_time_and_message(now, "updated time");
}

/// @brief initializes the ntp client
void initialize_sntp(void)
{
    ESP_LOGI(TAG, "initializing ntp");

    // print out the time at boot
    time_t now = 0;
    time(&now);
    print_time_and_message(now, "time at boot");

    // initialize the sntp
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_sync_interval(1); // minimum 15 seconds
    esp_sntp_setservername(0, SNTP_SERVER);
    esp_sntp_init();
    sntp_set_time_sync_notification_cb(on_got_time);
}

/// @brief stops the ntp client
void stop_sntp()
{
    ESP_LOGI(TAG, "stopping ntp");
    esp_sntp_stop();
}
