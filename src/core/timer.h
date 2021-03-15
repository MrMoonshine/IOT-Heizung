#ifndef TIMER_H
#define TIMER_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#ifndef MICRO_DENUM
    #define MICRO_DENUM 1000000
#endif

esp_err_t timerInit(void (*callback)());
#endif