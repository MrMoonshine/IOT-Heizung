#include "timer.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)

/*static void periodicTimerCallback(void* arg){
    printf("oida!\n");
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TIMERTAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
}*/

esp_err_t timerInit(const unsigned int timerinterval, void (*callback)()){
    if(esp_timer_init() == ESP_ERR_NO_MEM)
    return ESP_ERR_NO_MEM;

    esp_timer_create_args_t periodicTimerArgs = {
        .callback   = callback,
        .name       = "Meassure Temperatures"
    };

    esp_timer_handle_t periodicTimer;
    ESP_ERROR_CHECK(
        esp_timer_create(
            &periodicTimerArgs,
            &periodicTimer
        )
    );

    //Start timers
    esp_timer_start_periodic(
        periodicTimer,
        timerinterval*MICRO_DENUM
    );

    return ESP_OK;
}