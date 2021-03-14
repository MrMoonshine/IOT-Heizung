#include "tempsensor.h"

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

esp_err_t tempAnalogPolynom(int Rt){
    if(Rt < 0){
        ESP_LOGE(TAG,"Sensor disconnected or shortcutted!");
        return ESP_ERR_INVALID_ARG;
    }
    float temp = 0, r = (float)Rt;
    printf("Rt has %d Ohms\n",Rt);
    temp += -1.308e-9*pow(r,3);
    temp += 1.305e-5*pow(r,2);
    temp += -0.049*r;
    temp += 98.846;
    printf("T = %.2f °C\n",temp);
    return ESP_OK;
}