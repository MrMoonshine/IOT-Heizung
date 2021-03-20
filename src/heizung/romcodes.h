#ifndef ROMCODES_H
#define ROMCODES_H
#include <include/owb.h>
#include <include/owb_rmt.h>
#include <include/ds18b20.h>
/*---------------------------------------------------------*/
/*    Positionen Fürs Temperatur Array                     */
/*---------------------------------------------------------*/
#define SENSORS_TOTAL 9
#define TEMP_SOLAR SENSORS_TOTAL
#define TEMP_ROOM   0
#define TEMP_RED    1
#define TEMP_GREEN  2
#define TEMP_BLUE   3
#define TEMP_WHITE  4
#define TEMP_YELLOW 5
#define TEMP_BROWN  6
/*---------------------------------------------------------*/
/*              Farbe zu Positionen                        */
/*---------------------------------------------------------*/
#define TEMP_VORLAUF    TEMP_BLUE
#define TEMP_RUCKLAUF   TEMP_RED
#define TEMP_BUFFER     TEMP_WHITE
#define TEMP_HEIZUNG    TEMP_BROWN
/*---------------------------------------------------------*/
/*              ROM-CODES der Sensoren                     */
/*---------------------------------------------------------*/
static const OneWireBus_ROMCode roomrom = {
    .fields.family = {40},
    .fields.serial_number = {157,37,201,11,0,0},
    .fields.crc = {164}
};

static const OneWireBus_ROMCode redrom = {
    .fields.family = {40},
    .fields.serial_number = {170,82,197,78,20,1},
    .fields.crc = {111}
};

static const OneWireBus_ROMCode greenrom = {
    .fields.family = {40},
    .fields.serial_number = {170,122,245,78,20,1},
    .fields.crc = {225}
};

static const OneWireBus_ROMCode bluerom = {
    .fields.family = {40},
    .fields.serial_number = {170,161,197,78,20,1},
    .fields.crc = {135}
};

static const OneWireBus_ROMCode whiterom = {
    .fields.family = {40},
    .fields.serial_number = {170,137,188,78,20,1},
    .fields.crc = {122}
};

static const OneWireBus_ROMCode yellowrom = {
    .fields.family = {40},
    .fields.serial_number = {170,99,243,78,20,1},
    .fields.crc = {103}
};

static const OneWireBus_ROMCode brownrom = {
    .fields.family = {40},
    .fields.serial_number = {170,147,228,78,20,1},
    .fields.crc = {127}
};

/*---DEBUG SENSOR ADDRESSES---*/
//sensor 0: 4600000bc7446e28
static const OneWireBus_ROMCode debugtemp1rom = {
    .fields.family = {0x28},
    .fields.serial_number = {0x6e,0x44,0xc7,0x0b,0x00,0x00},
    .fields.crc = {0x46}
};
//sensor 1: ea00000bc9940128
static const OneWireBus_ROMCode debugtemp2rom = {
    .fields.family = {0x28},
    .fields.serial_number = {0x01,0x94,0xc9,0x0b,0x00,0x00},
    .fields.crc = {0xea}
};
#endif