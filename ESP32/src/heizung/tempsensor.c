#include "tempsensor.h"

static const char* TAG = "OneWireBus";

const float k = 273.15;

// Assign default values. Correct ones will be fetched via NTC-API of flash
static volatile float B = 4048.76;
static volatile float Tr = 25 + k;
static volatile float Rr = 4530;

void log_parameters(){
    ESP_LOGI(TAG,"B = %.2f; Tr = %.2f; Rr = %.2f", B, Tr, Rr);
}
// +-----------+--------+--------+--------+
// | variable: |   B    |   Tr   |   Rr   |
// +-----------+--------+--------+--------+
// | length:   | 4Bytes | 4Bytes | 4Bytes |
// +-----------+--------+--------+--------+
static const char* FLASH_STORAGE_NAMESPACE = "storage";
static const char* FLASH_PARAMETER_KEY = "NTC";
static const size_t FLASH_PARAMETER_SIZE = 3 * sizeof(float);
/*
    FLASH-ahh the hero of the universe!
    stores sensor parameters, to have them ready at next boot.
*/
esp_err_t flash_init(){
    //Using NVS to store factors
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGW(TAG, "Clearing flash...\tNVS storage contains no empty pages (which may happen if NVS partition was truncated)");
        
        err = nvs_flash_erase();
        if(err != ESP_OK)
            return err;

        err = nvs_flash_init();
        if(err != ESP_OK)
            return err;
    }
    return err;
}
// Single read operation
esp_err_t flash_read_values(){
    nvs_handle_t handle;
    esp_err_t err;

    // Open
    err = nvs_open(FLASH_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    size_t size;
    err = nvs_get_blob(handle, FLASH_PARAMETER_KEY, NULL, &size);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        ESP_LOGW(TAG, "Parameter wurden noch nicht im flash gespeichert. Abbruch.");
        return ESP_FAIL;
    }
    
    if(size != FLASH_PARAMETER_SIZE){
        ESP_LOGW(TAG, "Array in flash has wrong length of %d Bytes", size);
        return ESP_FAIL;
    }
    
    // Create blob buffer
    uint8_t* arr = (uint8_t*)malloc(FLASH_PARAMETER_SIZE);
    if(!arr)
        return ESP_FAIL;
    // Get array from flash and put it in float variables
    nvs_get_blob(handle, FLASH_PARAMETER_KEY, arr, &size);

    float tmp[3];
    for(uint8_t i = 0; i < 3; i++)
        memcpy(tmp + i, arr + i*sizeof(float), sizeof(float));

    B = tmp[0];
    Tr = tmp[1];
    Rr = tmp[2];
    log_parameters();

    free(arr);
    nvs_close(handle);
    return err;
}
// single write operation
esp_err_t flash_write_values(){
    nvs_handle_t handle;
    esp_err_t err;

    // Open
    err = nvs_open(FLASH_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    
    // Create blob buffer
    uint8_t* arr = (uint8_t*)malloc(FLASH_PARAMETER_SIZE);
    if(!arr)
        return ESP_FAIL;
 
    float tmp[3];
    tmp[0] = B;
    tmp[1] = Tr;
    tmp[2] = Rr;
    memcpy(arr, tmp, FLASH_PARAMETER_SIZE);

    err = nvs_set_blob(handle, FLASH_PARAMETER_KEY, arr, FLASH_PARAMETER_SIZE);
    if(err != ESP_OK){
        free(arr);
        return err;
    }

    free(arr);
    nvs_close(handle);
    return err;
}

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
#define ANALTEMP_SAMPLE_MEDIAN
#ifdef ANALTEMP_SAMPLE_MEDIAN
int compare(const void *a, const void *b){
    return (int)( *(unsigned int*)a - *(unsigned int*)b );
}

float median(unsigned int* values, size_t len){
    if(len < 1){
        return 0;
    }
    qsort(values, len, sizeof(unsigned int), compare);
    
    size_t center = len/2;
    if(len % 2){
        return values[center];
    }else{
        unsigned int ret = values[center++];
        ret += values[center];
        return ret / 2;
    }
}
#endif

#define ANALTEMP_SOLAR_RV  4694.0
#define ANALTEMP_SOLAR_VDD 3300.0

static const adc_unit_t unit = ADC_UNIT_1;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const unsigned short adc_read_len = 128;
//GPIO 36
static const adc_channel_t channel = ADC1_CHANNEL_0;
static adc_continuous_handle_t adc_handle;

int tempGetRt(uint32_t* v_i, int* Rt_i){
    uint32_t real_len = 0;

    uint8_t adc_buffer[adc_read_len];
    // Read ADC
    esp_err_t err = adc_continuous_read(adc_handle, adc_buffer, adc_read_len, &real_len, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    ESP_LOGI(TAG, "Real len is: %u", (unsigned int)real_len);
    uint32_t adc_raw = 0;
    #ifdef ANALTEMP_SAMPLE_MEDIAN
    unsigned int* outputs[adc_read_len];

    for (int i = 0; i < real_len; i += SOC_ADC_DIGI_RESULT_BYTES){
        uint16_t output = 0;
        memcpy(&output, adc_buffer + i, sizeof(output));
        //printf("hex: %x%x\toutput: (%x), ",(unsigned int)adc_buffer[i], (unsigned int)adc_buffer[i+1], output);
        //printf("out-dec: %u\n", output);
        outputs[i] = output;
    }
    printf("\n");
    adc_raw = median(outputs, adc_read_len);
    //ESP_LOGI(TAG, "ADC Raw is hex: %x\tdec: %u", (unsigned int)adc_raw, (unsigned int)adc_raw);
    #else 
    //printf("ADC Data Samples: ");
    for (int i = 0; i < real_len; i += SOC_ADC_DIGI_RESULT_BYTES){
        uint16_t output = 0;
        memcpy(&output, adc_buffer + i, sizeof(output));
        //printf("%x%x (%x), ",(unsigned int)adc_buffer[i], (unsigned int)adc_buffer[i+1], output);
        //printf("%u, ", output);
        adc_raw += output;
    }
    printf("\n");
    adc_raw /= (real_len / SOC_ADC_DIGI_RESULT_BYTES);
    #endif
    unsigned int adc_max = (1 << SOC_ADC_DIGI_MAX_BITWIDTH) - 1;
    //ESP_LOGI(TAG, "ADC Raw is %u\tpercentage: %.2f", (unsigned int)adc_raw, (float)(100*(float)adc_raw/(float)adc_max));


    uint32_t vin = adc_raw * ANALTEMP_SOLAR_VDD / adc_max;
    // Apparently the ADC measurres around 155mV below the actual voltage
    vin += 150;
    ESP_LOGI(TAG, "V = %u mV", (unsigned int)vin);
    if(v_i)
        *v_i = vin;
    if(vin >= ANALTEMP_SOLAR_VDD)
    return -1;
    
    int Rt = (float)(vin*ANALTEMP_SOLAR_RV)/(float)(ANALTEMP_SOLAR_VDD-vin);
    if(Rt_i)
        *Rt_i = Rt;

    return Rt;
}

void temp_analog_init(){
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 128,
        .conv_frame_size = adc_read_len
    };

    if(adc_continuous_new_handle(&adc_config, &adc_handle) != ESP_OK){
        ESP_LOGE(TAG, "ADC Init failure!");
        return;
    }

    adc_digi_pattern_config_t adc_pattern = {
        .atten = atten,
        .channel = channel,
        .unit = unit,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH
    };

    adc_continuous_config_t digi_conf = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 1,
        .adc_pattern = &adc_pattern
    };

    if(adc_continuous_config(adc_handle, &digi_conf) != ESP_OK){
        ESP_LOGE(TAG, "ADC Config failure!");
        return;
    }

    adc_continuous_start(adc_handle);


    if(flash_init() != ESP_OK){
        ESP_LOGW(TAG, "Flash init Error. Fallback to:");
        log_parameters();
        return;
    }

    flash_read_values();
}

float temp_analog_read(uint32_t* v_i, int* Rt_i){
    int Rt = tempGetRt(v_i, Rt_i);
    if(Rt < 0){
        ESP_LOGE(TAG,"Sensor disconnected or shortcutted!");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG,"Rt has %d Ohms",Rt);
    log_parameters();

    float p1 = log(Rt / Rr) / B + 1.0f/Tr;
    //ESP_LOGI(TAG, "denumerator is %.2f", p1);
    if(p1 == 0){
        ESP_LOGE(TAG,"Durch \"0\" dividiert!");
        return ESP_ERR_INVALID_ARG;
    }
    //Final Calculation
    float temp = (1.0f / p1);
    temp -= k;  // Kelvin to °C

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
    if(!reply){
        httpd_resp_send_500(req);
        ESP_LOGE(TAG, "Failed to allocate reply buffer!");
        return ESP_FAIL;
    }
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
    sprintf(tj, "\"solar\":%.2f}", temp_analog_read(NULL, NULL));
    strcat(reply, tj);
    free(tj);

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));

    free(reply);
    return ESP_OK;
}

esp_err_t heizung_api_ntc(httpd_req_t *req){
    /*
        Receives HTTP header data + does some Error handling
    */
    if(!rest_api_recv(req))
        return ESP_FAIL;

    int flashOK = 1;
    /*
        Handle GET variables
    */
    size_t queryLen = httpd_req_get_url_query_len(req) + 1;
    char* query = NULL;
    if(queryLen > 1){
        query = (char*)malloc(queryLen);
        if(!query){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        // Get the query
        httpd_req_get_url_query_str(req, query, queryLen);
        //Handle query
        const size_t rvbuffLen = 24;
        char* rvbuff = (char*)malloc(rvbuffLen);
        if(rvbuff == NULL){
            httpd_resp_send_500(req);
            free(query);
            return ESP_FAIL;
        }
        // Set parameters if sent as GET var
        if(httpd_query_key_value(query, "B", rvbuff, rvbuffLen) == ESP_OK){
            float tmp = atof(rvbuff);
            if(tmp != 0)
                B = tmp;
        }
        if(httpd_query_key_value(query, "Tr", rvbuff, rvbuffLen) == ESP_OK){
            float tmp = atof(rvbuff);
            if(tmp != 0)
                Tr = tmp;
        }
        if(httpd_query_key_value(query, "Rr", rvbuff, rvbuffLen) == ESP_OK){
            float tmp = atof(rvbuff);
            if(tmp != 0)
                Rr = tmp;
        }
        free(rvbuff);
        free(query);
        // write to flash
        flashOK = flash_write_values() == ESP_OK ? 0 : -1;
    }

    const char* parameters_pattern = "{\"B\":%.2f,\"Tr\":%.2f,\"Rr\":%.2f,\"flashOK\":%d}";
    // pattern length + 3 times max float lenght
    size_t replyLen = strlen(parameters_pattern) + 3*FLT_MAX_10_EXP + 4;
    char* reply = (char*)malloc(replyLen);
    if(!reply){
        httpd_resp_send_500(req);
        ESP_LOGE(TAG, "Failed to allocate reply buffer!");
        return ESP_FAIL;
    }

    sprintf(reply, parameters_pattern, B, Tr, Rr, flashOK);

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));
    free(reply);
    return ESP_OK;
}