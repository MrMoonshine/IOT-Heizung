#include "tempsensor.h"

static const char* OWBTAG = "OneWireBus";

bool tempVerify(OneWireBus *bus,OneWireBus_ROMCode *dev){
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
    return true;
}

esp_err_t buildsens(Temperature *tmp, const OneWireBus_ROMCode *rc, const char* name){
    tmp->name = (char*)malloc(strlen(name));
    if(!tmp->name)
    return ESP_ERR_NO_MEM;

    strcpy(tmp->name,name);
    tmp->romcode = rc;

    tempInitSensor(
        tmp->bus,
        (OneWireBus_ROMCode *)tmp->romcode,
        &tmp->info
    );
    
    if(!tempVerify(tmp->bus,(OneWireBus_ROMCode *)tmp->romcode)){
        ESP_LOGW(OWBTAG,"Der Sensor \"%s\" konnte nicht gefunden werden!",tmp->name);
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

esp_err_t tempBuildSensPtr(OneWireBus *bus,Temperature *temps){
    for(uint8_t a = 0; a < SENSORS_TOTAL; a++){
        temps[a].bus = bus;
    }
    Temperature* ptrt;
    ptrt = temps;

    /*ESP_ERROR_CHECK(buildsens(ptrt++,&roomrom,"room"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&redrom,"red"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&greenrom,"green"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&bluerom,"blue"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&whiterom,"white"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&yellowrom,"yellow"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&brownrom,"brown"));*/

    ESP_ERROR_CHECK(buildsens(ptrt++,&debugtemp1rom,"debug1"));
    ESP_ERROR_CHECK(buildsens(ptrt++,&debugtemp2rom,"debug2"));
    
    return ESP_OK;
}

OneWireBus* tempInitBus(gpio_num_t pin, owb_rmt_driver_info *rmtDriverInfo){
    //Initialize One Wire Bus
    return owb_rmt_initialize(
        rmtDriverInfo,
        pin,
        RMT_CHANNEL_1,
        RMT_CHANNEL_0
    ); 
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
    //Crashes Here
    if(!tempVerify(bus,code))
    return ESP_ERR_NOT_FOUND;

    ds18b20_init(info,bus,*code);
    ds18b20_use_crc(info, true);  //Enable CRC on all reads
    ds18b20_set_resolution(info,DS_RESOLUTION);
    return ESP_OK;
}

esp_err_t tempReadSensor(OneWireBus *bus, DS18B20_Info *info, float *temperature){
    ds18b20_convert_all(bus);
    ds18b20_wait_for_conversion(info);

    if(ds18b20_read_temp(info,temperature) == DS18B20_ERROR_DEVICE)
    return ESP_FAIL;

    return ESP_OK;
}

esp_err_t tempReadAll(float* temps, char* url){
    float temp = 0;
    tempAnalogPolynom(tempGetRt(),&temp);
    sprintf(url,"%s?solar=%.2f",TEMP_URL,temp);
    return ESP_OK;
}