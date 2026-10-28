/*
 * frame_dump.c - see frame_dump.h.
 */
#include "frame_dump.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void frame_dump_base64(const uint8_t *d, size_t n, int w, int h, const char *fmt)
{
    /* Silence other tasks' logs so they can't interleave into the base64. */
    esp_log_level_t prev = esp_log_level_get("*");
    esp_log_level_set("*", ESP_LOG_NONE);

    printf("\n---FRAME_DUMP_BEGIN w=%d h=%d len=%u fmt=%s---\n",
           w, h, (unsigned)n, fmt ? fmt : "i420");

    char line[80];
    int col = 0;
    unsigned lines = 0;
    size_t i = 0;
    for (; i + 3 <= n; i += 3) {
        uint32_t v = ((uint32_t)d[i] << 16) | ((uint32_t)d[i + 1] << 8) | d[i + 2];
        line[col++] = B64[(v >> 18) & 63];
        line[col++] = B64[(v >> 12) & 63];
        line[col++] = B64[(v >> 6)  & 63];
        line[col++] = B64[v & 63];
        if (col >= 76) {
            line[col++] = '\n'; line[col] = 0; fputs(line, stdout); col = 0;
            /* This dump takes seconds over UART; yield periodically so the idle
             * task runs and the task watchdog doesn't fire mid-dump (its panic
             * output isn't an ESP_LOG, so it can't be silenced - it must be
             * prevented). */
            if ((++lines & 0x3F) == 0) vTaskDelay(1);
        }
    }
    size_t rem = n - i;
    if (rem == 1) {
        uint32_t v = (uint32_t)d[i] << 16;
        line[col++] = B64[(v >> 18) & 63];
        line[col++] = B64[(v >> 12) & 63];
        line[col++] = '='; line[col++] = '=';
    } else if (rem == 2) {
        uint32_t v = ((uint32_t)d[i] << 16) | ((uint32_t)d[i + 1] << 8);
        line[col++] = B64[(v >> 18) & 63];
        line[col++] = B64[(v >> 12) & 63];
        line[col++] = B64[(v >> 6)  & 63];
        line[col++] = '=';
    }
    if (col) { line[col++] = '\n'; line[col] = 0; fputs(line, stdout); }

    printf("---FRAME_DUMP_END---\n");
    fflush(stdout);
    esp_log_level_set("*", prev);
}