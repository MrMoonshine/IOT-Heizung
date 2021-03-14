#include "recovery.h"

//Debug Name
static const char* RECTAG = "Recovery";
//Pumpenname im speicher
static const char* PUMPSV = "Pumps";

esp_err_t recoveryStartup(int8_t *data){
    nvs_handle_t my_handle;
    esp_err_t err = nvs_flash_init();
    if(err != ESP_OK){
        ESP_LOGW(RECTAG,"Failed to initialize Recovery");
        return err;
    }
    err = nvs_open(RECTAG,NVS_READWRITE,&my_handle);
    if(err != ESP_OK) return err;

    err = nvs_get_i8(my_handle,PUMPSV,data);
    nvs_close(my_handle);
    return err;
}

esp_err_t recoveryWrite(int8_t data){
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(RECTAG, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
    err = nvs_set_i8(my_handle,PUMPSV,data);
    if (err != ESP_OK) return err;

    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    nvs_close(my_handle);
    return err;
}