#include "tempsensor.h"

static const char *TAG = "OneWireBus";
// REST-API handle
static esp_http_client_handle_t client;
// constant to convert kelvin temperatures
const float k = 273.15;

volatile static float solar_thermocouple = 0.0f;
volatile static float solar_cold_junction = 0.0f;

// +-----------+--------+--------+--------+
// | variable: |   B    |   Tr   |   Rr   |
// +-----------+--------+--------+--------+
// | length:   | 4Bytes | 4Bytes | 4Bytes |
// +-----------+--------+--------+--------+
static const char *FLASH_STORAGE_NAMESPACE = "storage";
static const char *FLASH_PARAMETER_KEY = "NTC";
static const size_t FLASH_PARAMETER_SIZE = 3 * sizeof(float);
/*
    FLASH-ahh the hero of the universe!
    stores sensor parameters, to have them ready at next boot.
*/
esp_err_t flash_init()
{
    // Using NVS to store factors
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGW(TAG, "Clearing flash...\tNVS storage contains no empty pages (which may happen if NVS partition was truncated)");

        err = nvs_flash_erase();
        if (err != ESP_OK)
            return err;

        err = nvs_flash_init();
        if (err != ESP_OK)
            return err;
    }
    return err;
}
// Single read operation
esp_err_t flash_read_values()
{
    nvs_handle_t handle;
    esp_err_t err;

    // Open
    err = nvs_open(FLASH_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;
    size_t size;
    err = nvs_get_blob(handle, FLASH_PARAMETER_KEY, NULL, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Parameter wurden noch nicht im flash gespeichert. Abbruch.");
        return ESP_FAIL;
    }

    if (size != FLASH_PARAMETER_SIZE)
    {
        ESP_LOGW(TAG, "Array in flash has wrong length of %d Bytes", size);
        return ESP_FAIL;
    }

    // Create blob buffer
    uint8_t *arr = (uint8_t *)malloc(FLASH_PARAMETER_SIZE);
    if (!arr)
        return ESP_FAIL;
    // Get array from flash and put it in float variables
    nvs_get_blob(handle, FLASH_PARAMETER_KEY, arr, &size);

    float tmp[3];
    for (uint8_t i = 0; i < 3; i++)
        memcpy(tmp + i, arr + i * sizeof(float), sizeof(float));

    free(arr);
    nvs_close(handle);
    return err;
}
// single write operation
esp_err_t flash_write_values()
{
    nvs_handle_t handle;
    esp_err_t err;

    // Open
    err = nvs_open(FLASH_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    // Create blob buffer
    /*uint8_t *arr = (uint8_t *)malloc(FLASH_PARAMETER_SIZE);
    if (!arr)
        return ESP_FAIL;

    float tmp[3];
    tmp[0] = B;
    tmp[1] = Tr;
    tmp[2] = Rr;
    memcpy(arr, tmp, FLASH_PARAMETER_SIZE);

    err = nvs_set_blob(handle, FLASH_PARAMETER_KEY, arr, FLASH_PARAMETER_SIZE);
    if (err != ESP_OK)
    {
        free(arr);
        return err;
    }

    free(arr);*/
    nvs_close(handle);
    return err;
}

/*
    @brief sensor pointer
*/
static heizung_temperatur_t *temperaturen;

static bool tempVerify(OneWireBus *bus, OneWireBus_ROMCode *dev)
{
    bool isPresent = false;

    owb_status searchStatus = owb_verify_rom(
        bus,
        *dev,
        &isPresent);

    if (searchStatus != OWB_STATUS_OK)
    {
        ESP_LOGE(TAG, "Sensor Suche fehlgeschlagen");
        return false;
    }

    if (!isPresent)
    {
        char *devCode;
        devCode = (char *)malloc(OWB_ROM_CODE_STRING_LENGTH);
        if (devCode == NULL)
            return false;
        owb_string_from_rom_code(*dev, devCode, sizeof(devCode));
        ESP_LOGW(TAG, "Sensor nicht gefunden: %s", devCode);

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
static float tempReadSensor(DS18B20_Info *info)
{
    float temperature = TEMPERATURE_FAIL;
    if (!info->bus /*|| !&info->rom_code*/)
    {
        ESP_LOGW(TAG, "DS info ist NULL");
        return temperature;
    }
    else if (!tempVerify((OneWireBus *)info->bus, &info->rom_code))
    {
        // ESP_LOGW(TAG,"Den Sensor gibt es nicht");
        return temperature;
    }

    ds18b20_convert_all(info->bus);
    ds18b20_wait_for_conversion(info);

    if (ds18b20_read_temp(info, &temperature) == DS18B20_ERROR_DEVICE)
    {
        ESP_LOGW(TAG, "DS18B20 Device Error");
        return TEMPERATURE_FAIL;
    }

    return temperature;
}

static esp_err_t temp_owb_init_sensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info)
{
    ds18b20_init(info, bus, *code);
    ds18b20_use_crc(info, true); // Enable CRC on all reads
    if (ds18b20_set_resolution(info, DS_RESOLUTION))
    {
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t temp_owb_add_sensor(heizung_temperatur_t **head, OneWireBus *bus, OneWireBus_ROMCode *code, uint8_t id, const char *name)
{
    heizung_temperatur_t *elem = (heizung_temperatur_t *)malloc(sizeof(heizung_temperatur_t));
    if (elem == NULL)
        return ESP_FAIL;
    // Assign Data
    memset(elem->name, 0, TEMPSENSOR_NAME_LEN);
    // Just to be safe
    if (strlen(name) > TEMPSENSOR_NAME_LEN)
        return ESP_FAIL;
    // Copy
    strcpy(elem->name, name);
    elem->romcode = code;
    elem->value = 0.0f;
    elem->id = id;
    // Init sensor
    temp_owb_init_sensor(bus, elem->romcode, &elem->info);
    // Next node is head
    elem->next = *head;
    // this is the new First node
    *head = elem;
    return ESP_OK;
}

esp_err_t temp_owb_add_all(OneWireBus *bus)
{
    temperaturen = NULL;
    esp_err_t err = ESP_OK;
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&roomrom, TEMP_ROOM, "room");
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&bluerom, TEMP_BLUE, "blue");
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&redrom, TEMP_RED, "red");
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&greenrom, TEMP_GREEN, "green");
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&yellowrom, TEMP_YELLOW, "yellow");
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&whiterom, TEMP_WHITE, "white");
    err |= temp_owb_add_sensor(&temperaturen, bus, (OneWireBus_ROMCode *)&brownrom, TEMP_BROWN, "brown");
    return err;
}

esp_err_t temp_owb_read_sensors()
{
    heizung_temperatur_t *elem = temperaturen;
    while (elem)
    {
        elem->value = tempReadSensor(&elem->info);
        ESP_LOGI(TAG, "T%s = %.2f", elem->name, elem->value);
        elem = elem->next;
    }
    return ESP_OK;
}

unsigned int temp_owb_count_sensors()
{
    unsigned int ret = 0;
    heizung_temperatur_t *elem = temperaturen;
    while (elem)
    {
        ret++;
        elem = elem->next;
    }
    return ret;
}

heizung_temperatur_t *temp_owb_list()
{
    return temperaturen;
}

void tempDoSettings(OneWireBus *owb)
{
    // Enable CRC
    owb_use_crc(owb, true);

    // Check for parasitric devices
    bool parasitic_power = false;
    ds18b20_check_for_parasite_power(owb, &parasitic_power);
    if (parasitic_power)
    {
        printf("Parasitic-powered devices detected\n");
    }
    ESP_LOGI(TAG, "Parasitic power is %sabled", parasitic_power ? "en" : "dis");
    owb_use_parasitic_power(owb, parasitic_power);
#ifdef CONFIG_ENABLE_STRONG_PULLUP_GPIO
    // An external pull-up circuit is used to supply extra current to OneWireBus devices
    // during temperature conversions.
    owb_use_strong_pullup_gpio(owb, CONFIG_STRONG_PULLUP_GPIO);
#endif
}

/*
* @brief helper function to split raw payload in two values
*/
static void payloadtof(char* payload, float* thermocouple, float* cj){
    char* token = strtok(payload, "\n");
    uint8_t counter = 0;
    while(token != NULL && counter < 2){
        switch (counter++)
        {
        case 0:
            *thermocouple = atoff(token);
            break;
        case 1:
            *cj = atoff(token);
            break;
        default:
            break;
        }
        token = strtok(NULL, "\n");
    }
}

static esp_err_t solar_api_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "Got Data! Length: %u", evt->data_len);
        //ESP_LOGI(TAG, "%s", (char *)evt->user_data);
        if(esp_http_client_is_chunked_response(evt->client)){
            ESP_LOGE(TAG, "Got chunked DATA from %s ! Not allowed!", TEMPSENSOR_SOLAR_URL);
            return ESP_FAIL;
        }

        int64_t contentLen = esp_http_client_get_content_length(evt->client);
        char* content = (char*)malloc(contentLen);
        if(!content){
            ESP_LOGE(TAG, "malloc error!");
            return ESP_FAIL;
        }

        memcpy(content, evt->data, contentLen);
        ESP_LOGI(TAG, "got content: %s", content);
        payloadtof(content, &solar_thermocouple, &solar_cold_junction);
        ESP_LOGI(TAG, "TC = %.2f", solar_thermocouple);
        ESP_LOGI(TAG, "CJ = %.2f", solar_cold_junction);

        free(content);
        break;
    default:
        break;
    }

    return ESP_OK;
}

esp_err_t temp_rest_init()
{
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_GET,
        .url = TEMPSENSOR_SOLAR_URL,
        .event_handler = solar_api_event_handler,
        .is_async = true
    };
    client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to init REST-API-Client");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t temp_rest_read()
{
    ESP_LOGI(TAG, "Reading Solar-Sensor...");
     esp_err_t err = esp_http_client_perform(client);
     if(err != ESP_OK){
        ESP_LOGE(TAG, "%s\tHTTP Error: %d\n", TEMPSENSOR_SOLAR_URL , esp_http_client_get_status_code(client));
        return err;
     }
    return ESP_OK;
}

static esp_err_t api_helper_ftoa(char *buffer, size_t bufferLen, const char *name, float value)
{
    memset(buffer, 0, bufferLen);
    // To prevent a possible crash
    sprintf(buffer, "\"%s\":%.2f,", name, abs(value) > 3000 ? TEMPERATURE_FAIL : value);
    return ESP_OK;
}

esp_err_t heizung_api_temperatures(httpd_req_t *req)
{
    /*
        Receives HTTP header data + does some Error handling
    */
    if (!rest_api_recv(req))
        return ESP_FAIL;

    const char *json_template_end = "\"status\":0}";

    const size_t templen = 24;
    size_t replylen = (2 + temp_owb_count_sensors()) * templen + 1 + strlen(json_template_end);
    char *reply = (char *)malloc(replylen);
    if (!reply)
    {
        httpd_resp_send_500(req);
        ESP_LOGE(TAG, "Failed to allocate reply buffer!");
        return ESP_FAIL;
    }
    memset(reply, 0, replylen);
    strcpy(reply, "{");

    char *tj = (char *)malloc(templen);
    if (!tj)
    {
        httpd_resp_send_500(req);
        ESP_LOGE(TAG, "Failed to allocate reply buffer!");
        return ESP_FAIL;
    }
    // API Sensor
    //temp_rest_read(&thermocouple, &cold_junction);
    //ESP_LOGI(TAG, "TC: %.2f °C\tCJ: %.2f °C", thermocouple, cold_junction);

    api_helper_ftoa(tj, templen, "solar", solar_thermocouple);
    strcat(reply, tj);
    api_helper_ftoa(tj, templen, "cold_junction", solar_cold_junction);
    strcat(reply, tj);

    // Handle DS18B20 Sensors
    heizung_temperatur_t *elem = temperaturen;
    while (elem)
    {
        // sprintf(tj, "\"%s\":%.2f,", elem->name, elem->value);
        api_helper_ftoa(tj, templen, elem->name, elem->value);
        strcat(reply, tj);
        elem = elem->next;
    }
    free(tj);
    // Append status var in JSON
    strcat(reply, json_template_end);

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));

    free(reply);
    return ESP_OK;
}

float temp_get_solar(){
    return solar_thermocouple;
}