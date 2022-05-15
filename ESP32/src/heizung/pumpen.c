#include "pumpen.h"

extern rest_user_t *rest_api_users;

static const char* PUMPTAG = "Pumpensteuerung";
/*-------------Relay Board 1-------------------*/
//Pumpe für die Heizung
static const Pumpe heizpumpe = {
    .name = "heizpumpe",
    .gpio = 15,
    .mask = 1 << 0,
};
//Solarpumpe
static const Pumpe solarpumpe = {
    .name = "solarpumpe",
    .gpio = 2,
    .mask = 1 << 1,
};
//Ungenutztes Relay
static const Pumpe redundancy1 = {
    .name = "unbenutzt1",
    .gpio = 4,
    .mask = 1 << 2,
};
//Bufferpumpe
static const Pumpe bufferpumpe = {
    .name = "bufferpumpe",
    .gpio = 16,
    .mask = 1 << 3,
};
/*-------------Relay Board 2-------------------*/
//Mixer1
//Mixer2
//Zwischenpumpe
static const Pumpe zwischenpumpe = {
    .name = "zwischenpumpe",
    .gpio = 18,
    .mask = 1 << 4,
};
//Wärepumpe
static const Pumpe warmepumpe = {
    .name = "waermepumpe",
    .gpio = 19,
    .mask = 1 << 5,
};
//There are 6 Pumps in total
static const Pumpe* allpumps[PUMPENANZAHL] = {
    &heizpumpe,
    &solarpumpe,
    &redundancy1,
    &bufferpumpe,
    &zwischenpumpe,
    &warmepumpe,
};

static void initPump(const Pumpe *pump){
    gpio_pad_select_gpio(pump->gpio);
    gpio_reset_pin(pump->gpio); 
    gpio_set_direction(pump->gpio, GPIO_MODE_INPUT_OUTPUT);
}

esp_err_t pumpsInit(){
    // Alle aus weil invertiert
    int8_t states = 0xFF;
    // Alle standardmäßigen pumpen einschalten
    states &= ~bufferpumpe.mask;
    states &= ~heizpumpe.mask;
    states &= ~warmepumpe.mask;
    states &= ~zwischenpumpe.mask;

    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        initPump(allpumps[a]);
        gpio_set_level(allpumps[a]->gpio, PUMP_OFF);
    }

    ESP_LOGI(PUMPTAG,"Initial States: %d",states);
    pumpsWrite(states);
    return ESP_OK;
}

pump_states_t pumpsDefault(){
    return (0 | bufferpumpe.mask | heizpumpe.mask | zwischenpumpe.mask | warmepumpe.mask);
}

esp_err_t pumpsWrite(pump_states_t states){
    if(states < 0)
        return ESP_FAIL;
    //printf("State is %d\nMask is %d\n",states,allpumps[1]->mask);
    ESP_LOGI(PUMPTAG, "Setting states to: %x\n", states);
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        //Solarpumpe ignorieren
        if(
            allpumps[a]->gpio == solarpumpe.gpio ||
            allpumps[a]->gpio == redundancy1.gpio
        ){
            gpio_set_level(redundancy1.gpio, PUMP_OFF); // keep the relay shut. unessecary power loss
            continue;
        }
        //Die Logik Level für die Pumpen sind Invertiert
        gpio_set_level(
            allpumps[a]->gpio,
            allpumps[a]->mask & states ? PUMP_OFF : PUMP_ON
        );
        //ESP_LOGI(PUMPTAG, "%s ist jetzt %s", allpumps[a]->name, allpumps[a]->mask & states ? "Ein" : "Aus");
    }
    return ESP_OK;
}

pump_states_t pumpsRead(){
    int8_t states = 0;
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        states |= gpio_get_level(allpumps[a]->gpio) ? 0 : allpumps[a]->mask;
    }
    return states;
}

esp_err_t heizung_api_pumps(httpd_req_t *req){
    /*
        Receives HTTP header data + does some Error handling
    */
    if(!rest_api_recv(req))
        return ESP_FAIL;
    /*
        A single function that handles authentication and error replies towards the client 
    */
    if(!rest_api_authenticate(req, rest_api_users, REST_USER_PERMISSION_RW))
        return ESP_FAIL;

    int8_t status = 0;
    const char* json_tmp_pump_all = "{\"status\":%.1d, \"pumps\":[%s]}";
    const char* json_tmp_pump_single = "{\"name\":\"%s\", \"state\":%.1d}";

    size_t parrebuff = strlen(json_tmp_pump_single) + 24 + 2;
    char* parr = (char*)malloc(parrebuff * PUMPENANZAHL);
    if(!parr){
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    strcpy(parr, "");
    // Build JSON
    char* parre = (char*)malloc(parrebuff);
    if(!parre){
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        /*
            Headervariablen überprüfen um Pumpe zu schalten
        */
        size_t len = httpd_req_get_hdr_value_len(req, allpumps[a]->name);
        len++;
        // Don't care about the useless relay
        if(len > 0 && allpumps[a]->gpio != redundancy1.gpio){
            char* rvbuff = (char*)malloc(len);
            if(rvbuff == NULL){
                httpd_resp_send_500(req);
                free(parr);
                free(parre);
                return ESP_FAIL;
            }
            esp_err_t err = httpd_req_get_hdr_value_str(req, allpumps[a]->name, rvbuff, len);
            if(err == ESP_OK){
                int gs = atoi(rvbuff);
                gpio_set_level(allpumps[a]->gpio, gs ? PUMP_OFF : PUMP_ON);
                ESP_LOGI(PUMPTAG, "%s ist jetzt %s", allpumps[a]->name, gs ? "Aus" : "Ein");
            }
        }

        // Creating JSON
        strcpy(parre, "");
        // Pumps are inverted
        uint8_t pstatus = gpio_get_level(allpumps[a]->gpio) ? PUMP_OFF : PUMP_ON;
        sprintf(parre, json_tmp_pump_single, allpumps[a]->name, pstatus);
        strcat(parr, parre);
        if(a != PUMPENANZAHL - 1)
            strcat(parr, ",");
    }
    free(parre);
    char* reply = (char*)malloc(strlen(json_tmp_pump_all) + parrebuff * PUMPENANZAHL);
    strcpy(reply, "");
    sprintf(reply, json_tmp_pump_all, status, parr);
    free(parr);

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));

    free(reply);
    return ESP_OK;
}