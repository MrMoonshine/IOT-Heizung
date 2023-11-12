#ifndef SOLAR_H
#define SOLAR_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "romcodes.h"
#include "pumpen.h"
#include "tempsensor.h"
#include "../wifi.h"
/*---------------------------------------------------------*/
/*              Solar Funktionen                           */
/*---------------------------------------------------------*/
#define SOLAR_HTTP_BUFFER_SIZE 64
#define SOLAR_BLOCK_CYCLE_AFTER_ENABLE 1

//Diese Differenz muss das solarpannel erreichen um die Pumpe zum einschalten
static const float SOLAR_TO_BUFF_OFFSET = 10; //°C
// Schutz, damit die pumpe nicht einschaltet wenn es zu kalt ist. Die B-Formel ist exponential, und um richtige messwerte zu haben, gibt es den schutz.
static const float SOLAR_ENABLE_MIN_TEMP = 40.0f; //°C
/**
 * @brief  Diese Funktion steuert die Solarpumpe mit den gegebenen Temperaturen
 *
 * @param  temps Alle gemessenen Temperaturen der Sensoren
 *
 * @return Anzahl der Berechnungen zum Aussetzen
*/
uint8_t solarSetByTemp(float solar, float buffer, float vorlauf, float rucklauf);
#endif