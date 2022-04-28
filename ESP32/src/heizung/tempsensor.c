#include "tempsensor.h"

static const char* OWBTAG = "OneWireBus";
static const char *sensornames[SENSORS_TOTAL+1] = {
    "room",
    "red",
    "green",
    "blue",
    "white",
    "yellow",
    "brown",
    //"debug1",
    //"debug2",
    "solar"
};

static const OneWireBus_ROMCode *sensorcodes[SENSORS_TOTAL] = {
    &roomrom,
    &redrom,
    &greenrom,
    &bluerom,
    &whiterom,
    &yellowrom,
    &brownrom,
    //&debugtemp1rom,
    //&debugtemp2rom
};

bool tempVerify(OneWireBus *bus, OneWireBus_ROMCode *dev){
    bool isPresent = false;
    
    owb_status searchStatus = owb_verify_rom(
        bus,
        *dev,
        &isPresent
    );

    if(searchStatus != OWB_STATUS_OK){
        ESP_LOGE(OWBTAG,"Failed to search device");
        return false;
    }
    
    if(!isPresent){
        char *devCode;
        devCode = (char*)malloc(OWB_ROM_CODE_STRING_LENGTH);
        if(devCode == NULL)
        return false;
        owb_string_from_rom_code(*dev,devCode,sizeof(devCode));
        ESP_LOGW(OWBTAG,"Failed to find sensor: %s",devCode);

        free(devCode);
        return false;
    }
    return isPresent;
}

esp_err_t tempBuildSensPtr(OneWireBus *bus, DS18B20_Info *sensors){
    esp_err_t ret = ESP_OK;
    for(uint8_t a = 0; a < SENSORS_TOTAL; a++){
        if(tempInitSensor(bus,(OneWireBus_ROMCode *)sensorcodes[a],sensors+a) != ESP_OK){
            ESP_LOGE(OWBTAG,"Sensor %s konnte nicht gefunden werden!",sensornames[a]);
            ret = ESP_ERR_NOT_FOUND;
        }
    }
    return ret;
}

void tempDoSettings(OneWireBus *owb){
    //Enable CRC
    owb_use_crc(owb, true);

    //Check for parasitric devices
    bool parasitic_power = false;
    ds18b20_check_for_parasite_power(owb, &parasitic_power);
    if (parasitic_power) {
        printf("Parasitic-powered devices detected\n");
    }

    owb_use_parasitic_power(owb, parasitic_power);
#ifdef CONFIG_ENABLE_STRONG_PULLUP_GPIO
    // An external pull-up circuit is used to supply extra current to OneWireBus devices
    // during temperature conversions.
    owb_use_strong_pullup_gpio(owb, CONFIG_STRONG_PULLUP_GPIO);
#endif
}

esp_err_t tempInitSensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info){
    ds18b20_init(info,bus,*code);
    ds18b20_use_crc(info, true);  //Enable CRC on all reads
    if(ds18b20_set_resolution(info,DS_RESOLUTION)){
        return ESP_OK;
    }    
    return ESP_ERR_NOT_FOUND;
}

float tempReadSensor(DS18B20_Info *info){
    float temperature = TEMPERATURE_FAIL;
    if(!info->bus || !&info->rom_code){
        ESP_LOGW(OWBTAG,"DS info ist NULL");
        return temperature;
    }else if(!tempVerify((OneWireBus*)info->bus,&info->rom_code)){
        //ESP_LOGW(OWBTAG,"Den Sensor gibt es nicht");
        return temperature;
    }

    ds18b20_convert_all(info->bus);
    ds18b20_wait_for_conversion(info);
    
    if(ds18b20_read_temp(info,&temperature) == DS18B20_ERROR_DEVICE){
        ESP_LOGW(OWBTAG,"DS18B20 Device Error");
        return TEMPERATURE_FAIL;
    }

    return temperature;
}

esp_err_t tempReadArray(float* temps, DS18B20_Info *sensors){  
    //Read all DS18B20 Sensors
    for(uint8_t a = 0; a < SENSORS_TOTAL; a++){
        temps[a] = tempReadSensor(sensors + a);
    }

    //Read analog sensor
    return tempAnalogCalc(tempGetRt(),temps+SENSORS_TOTAL);
}

esp_err_t tempArray2URL(float* temps, char* url){
    esp_err_t ret = ESP_ERR_INVALID_RESPONSE;
    char varbuff[16] = ""; 
    strcpy(url,URL_TEMPERATURES);
    strcat(url,"?");

    for(uint8_t a = 0; a < SENSORS_TOTAL + 1; a++){
        strcat(url,sensornames[a]);
        strcat(url,"=");
        if(temps[a] > ZERO_KELVIN){
            sprintf(varbuff,"%.2f",temps[a]);
            strcat(url,varbuff);
            ret = ESP_OK;
        }

        if(a < SENSORS_TOTAL)
        strcat(url,"&");
    }
    return ret;
}

/*---------------------------------------
    Analog Temperature
---------------------------------------*/

#define ANALTEMP_SOLAR_RV  4694.0
#define ANALTEMP_SOLAR_VDD 3300.0

static const adc_unit_t unit = ADC_UNIT_1;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
//GPIO 36
static const adc_channel_t channel = ADC1_CHANNEL_0;
static esp_adc_cal_characteristics_t *calli;

static const char* TAG = "Analog Meassure";

int tempGetRt(){
    uint32_t vin = esp_adc_cal_raw_to_voltage(adc1_get_raw(channel),calli);
    if(vin >= ANALTEMP_SOLAR_VDD)
    return -1;
    
    return (float)(vin*ANALTEMP_SOLAR_RV)/(float)(ANALTEMP_SOLAR_VDD-vin);
}

void tempAnalogInit(){
    calli = calloc(1,sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(
        unit,
        atten,
        width,
        1100,   //because of atten
        calli
    );

    adc1_config_width(width);
    adc1_config_channel_atten(channel,atten);
}

esp_err_t tempAnalogCalc(int Rt,float *tempp){
    if(Rt < 0){
        ESP_LOGE(TAG,"Sensor disconnected or shortcutted!");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG,"Rt has %d Ohms\n",Rt);
    const float R25 = 4530;
    const float B = 4048.76;
    const float k = 273.15;
    const float T25 = 25 + k;

    float p1 = log(Rt / R25) / B + 1.0f/T25;
    if(p1 == 0){
        ESP_LOGE(TAG,"Durch \"0\" dividiert!");
        return ESP_ERR_INVALID_ARG;
    }
    //Final Calculation
    float temp = (1.0f / p1) - k;
    // Correct it a bit... aka cheating in math
    temp += 10;
    *tempp = temp;
    ESP_LOGI(TAG,"T = %.2f °C\n",temp);
    return ESP_OK;
}