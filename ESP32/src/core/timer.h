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
/*
    @brief Setzt einen Timer fix fertig auf
    @param[in] timeinterval interval in s
    @param[in] callback callback funktion
    @return ESP_OK on success
*/
esp_err_t timerInit(const unsigned int timerinterval, void (*callback)());
#endif