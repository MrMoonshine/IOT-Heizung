#ifndef PUMPEN_H
#define PUMPEN_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "recovery.h"
/*
    Der zustand von allen Pumpen wird im NVS flash gespeichert.
    Die Realys vom 4-Wege-Mischer werden nicht gespeichert.
*/
typedef struct{
    gpio_num_t gpio;
    int8_t mask;
}Pumpe;

static const char* PUMPTAG = "Pumpensteuerung";
//Relay boards sind invertiert
#define PUMP_ON     0
#define PUMP_OFF    1

/*-------------Relay Board 1-------------------*/
//Pumpe für die Heizung
static const Pumpe heizpumpe = {
    .gpio = 15,
    .mask = 1 << 0,
};
//Solarpumpe
static const Pumpe solarpumpe = {
    .gpio = 2,
    .mask = 1 << 1,
};
//Ungenutztes Relay
static const Pumpe redundancy1 = {
    .gpio = 4,
    .mask = 1 << 2,
};
//Bufferpumpe
static const Pumpe bufferpumpe = {
    .gpio = 16,
    .mask = 1 << 3,
};
/*-------------Relay Board 2-------------------*/
//Mixer1
//Mixer2
//Zwischenpumpe
static const Pumpe zwischenpumpe = {
    .gpio = 18,
    .mask = 1 << 4,
};
//Wärepumpe
static const Pumpe warmepumpe = {
    .gpio = 19,
    .mask = 1 << 5,
};
#define PUMPENANZAHL 6
//There are 6 Pumps in total
static const Pumpe* allpumps[PUMPENANZAHL] = {
    &heizpumpe,
    &solarpumpe,
    &redundancy1,
    &bufferpumpe,
    &zwischenpumpe,
    &warmepumpe,
};
/*-------------Functions-------------------*/
esp_err_t pumpsWrite(int8_t states);
esp_err_t pumpsInit();
#endif