#include "tempsensor.h"

static const char* TAG = "OneWireBus";

/*
    @brief sensor pointer
*/
static heizung_temperatur_t* temperaturen;

static bool tempVerify(OneWireBus *bus, OneWireBus_ROMCode *dev){
    bool isPresent = false;
    
    owb_status searchStatus = owb_verify_rom(
        bus,
        *dev,
        &isPresent
    );

    if(searchStatus != OWB_STATUS_OK){
        ESP_LOGE(TAG,"Sensor Suche fehlgeschlagen");
        return false;
    }
    
    if(!isPresent){
        char *devCode;
        devCode = (char*)malloc(OWB_ROM_CODE_STRING_LENGTH);
        if(devCode == NULL)
        return false;
        owb_string_from_rom_code(*dev,devCode,sizeof(devCode));
        ESP_LOGW(TAG,"Sensor nicht gefunden: %s",devCode);

        free(devCode);
        return false;
    }
    return isPresent;
}
/*
 * @brief Lesen der Temperatur eines Sensors
 * @param info Der Info Struct vom Sensor
 * @return Temperatur in °C. Bei Fehler kommt -2048°C
*/
static float tempReadSensor(DS18B20_Info *info){
    float temperature = TEMPERATURE_FAIL;
    if(!info->bus || !&info->rom_code){
        ESP_LOGW(TAG,"DS info ist NULL");
        return temperature;
    }else if(!tempVerify((OneWireBus*)info->bus,&info->rom_code)){
        //ESP_LOGW(TAG,"Den Sensor gibt es nicht");
        return temperature;
    }

    ds18b20_convert_all(info->bus);
    ds18b20_wait_for_conversion(info);
    
    if(ds18b20_read_temp(info,&temperature) == DS18B20_ERROR_DEVICE){
        ESP_LOGW(TAG,"DS18B20 Device Error");
        return TEMPERATURE_FAIL;
    }

    return temperature;
}

static esp_err_t temp_owb_init_sensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info){
    ds18b20_init(info, bus,*code);
    ds18b20_use_crc(info, true);  //Enable CRC on all reads
    if(ds18b20_set_resolution(info,DS_RESOLUTION)){
        return ESP_OK;
    }    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t temp_owb_add_sensor(heizung_temperatur_t** head, OneWireBus *bus, OneWireBus_ROMCode *code, uint8_t id, const char* name){
    heizung_temperatur_t *elem = (heizung_temperatur_t*) malloc(sizeof(heizung_temperatur_t));
    if(elem == NULL)
        return ESP_FAIL;
    //Assign Data
    memset(elem->name, 0, TEMPSENSOR_NAME_LEN);
    // Just to be safe
    if(strlen(name) > TEMPSENSOR_NAME_LEN)
        return ESP_FAIL;
    // Copy
    strcpy(elem->name, name);
    elem->romcode = code;
    elem->value = 0.0f;
    elem->id = id;
    // Init sensor
    temp_owb_init_sensor(bus, elem->romcode, &elem->info);
    //Next node is head
    elem->next = *head;
    // this is the new First node
    *head = elem;
    return ESP_OK;
}

esp_err_t temp_owb_add_all(OneWireBus *bus){
    temperaturen = NULL;
    esp_err_t err = ESP_OK;
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&roomrom, TEMP_ROOM, "room");
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&bluerom, TEMP_BLUE, "blue");
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&redrom, TEMP_RED, "red");
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&greenrom, TEMP_GREEN, "green");
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&yellowrom, TEMP_YELLOW, "yellow");
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&whiterom, TEMP_WHITE, "white");
    err |= temp_owb_add_sensor(&temperaturen,bus, (OneWireBus_ROMCode *)&brownrom, TEMP_BROWN, "brown");
    return err;
}

esp_err_t temp_owb_read_sensors(){
    heizung_temperatur_t *elem = temperaturen;
    while(elem){
        elem->value = tempReadSensor(&elem->info);
        ESP_LOGI(TAG, "T%s = %.2f", elem->name, elem->value);
        elem = elem->next;
    }
    return ESP_OK;
}

unsigned int temp_owb_count_sensors(){
    unsigned int ret = 0;
    heizung_temperatur_t *elem = temperaturen;
    while(elem){
        ret++;
        elem = elem->next;
    }
    return ret;
}

heizung_temperatur_t* temp_owb_list(){
    return temperaturen;
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

int tempGetRt(){
    uint32_t vin = esp_adc_cal_raw_to_voltage(adc1_get_raw(channel),calli);
    if(vin >= ANALTEMP_SOLAR_VDD)
    return -1;
    
    return (float)(vin*ANALTEMP_SOLAR_RV)/(float)(ANALTEMP_SOLAR_VDD-vin);
}

void temp_analog_init(){
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

float temp_analog_read(){
    int Rt = tempGetRt();
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
    ESP_LOGI(TAG,"T = %.2f °C\n",temp);
    return temp;
}

esp_err_t heizung_api_temperatures(httpd_req_t *req){
    /*
        Receives HTTP header data + does some Error handling
    */
    if(!rest_api_recv(req))
        return ESP_FAIL;

    const char* json_template = "{\"status\":0,";

    size_t templen = 24;
    size_t replylen = strlen(json_template) + (1 + temp_owb_count_sensors())*templen;
    char* reply = (char*)malloc(replylen);
    if(!reply)
        return ESP_FAIL;
    memset(reply, 0, replylen);
    strcpy(reply, json_template);

    char* tj = (char*)malloc(templen);
    heizung_temperatur_t *elem = temperaturen;
    while (elem){
        memset(tj, 0, templen);
        sprintf(tj, "\"%s\":%.2f,", elem->name, elem->value);
        strcat(reply, tj);
        elem = elem->next;
    }
    memset(tj, 0, templen);
    sprintf(tj, "\"solar\":%.2f}", temp_analog_read());
    strcat(reply, tj);
    free(tj);

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));

    free(reply);
    return ESP_OK;
}