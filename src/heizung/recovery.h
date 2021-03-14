#ifndef RECOVERY_H_
#define RECOVERY_H_
/*Die Recovery wird benutzt um,
 bei einem Reset die Pumpen sofort wieder ein
oder aus zu schalten, so wie sie vorher waren*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
//Jedes Bit gehört einer Pumpe.
esp_err_t recoveryStartup(int8_t *data);
//Write in Memory
esp_err_t recoveryWrite(int8_t data);
#endif