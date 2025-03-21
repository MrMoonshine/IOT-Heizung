#include "wifi.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//#define IPv6_STATIC_ADDR "fd00:420::1e"

//#define STATIC_IP_ADDR "10.0.0.31"
//#define STATIC_NETMASK_ADDR "255.255.255.0"
//#define STATIC_GW_ADDR "10.0.0.130"
#define HOSTNAME "solar"
#define IPv6_DNS_ADDR "fd00:420::5e49:79ff:fed4:b005"

static const char *TAG = "WiFi-Handler";
static EventGroupHandle_t wifi_event_group;
//Setup NVS
static void initNVS(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG,"Erasing NVS-flash due to failure in initialisation");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}
#ifdef IPv4_STATIC_GW
static void wifi_set_static_ip(esp_netif_t *netif)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(IPv4_STAIC_MGMT);
    ip.netmask.addr = ipaddr_addr(IPv4_STATIC_NETMASK);
    ip.gw.addr = ipaddr_addr(IPv4_STATIC_GW);

    esp_err_t err = esp_netif_set_ip_info(netif, &ip);
    switch (err){
        case ESP_ERR_ESP_NETIF_INVALID_PARAMS: ESP_LOGE(TAG, "Invalid parameters!"); break;
        case ESP_ERR_ESP_NETIF_DHCP_NOT_STOPPED: ESP_LOGE(TAG, "DHCP is still running!"); break;
        default:    break;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ip info! Fallback to DHCP");
        return;
    }
    ESP_LOGD(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", IPv4_STAIC_MGMT, IPv4_STATIC_NETMASK, IPv4_STATIC_GW);
}
#endif

#ifndef IPv4_DNS_ADDR
    #define IPv4_DNS_ADDR IPv4_STATIC_GW
#endif

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        //ESP_LOGI(TAG,"Attempt to set IP %s", IPv4_STAIC_MGMT);
        #ifdef IPv4_STATIC_GW
            wifi_set_static_ip(arg);
        #endif
        // Set Link-Local
        esp_netif_create_ip6_linklocal(arg);
        // Set Static IPv6 Address
        #ifdef IPv6_STATIC_ADDR
            esp_ip6_addr_t global;
            if(esp_netif_str_to_ip6(IPv6_STATIC_ADDR, &global) != ESP_OK){
                ESP_LOGE(TAG, "Invalid address %s !", IPv6_STATIC_ADDR);
                return;
            }
            esp_netif_add_ip6_address(arg, global, true);
        #endif

        // Set DNS Info
        esp_netif_dns_info_t dnsinfo;
        if(esp_netif_str_to_ip4(IPv4_DNS_ADDR, &dnsinfo.ip.u_addr.ip4) != ESP_OK){
            ESP_LOGE(TAG, "Invalid address %s !", IPv4_DNS_ADDR);
        }
        if(esp_netif_str_to_ip6(IPv6_DNS_ADDR, &dnsinfo.ip.u_addr.ip6) != ESP_OK){
            ESP_LOGE(TAG, "Invalid address %s !", IPv6_DNS_ADDR);
        }
        if(esp_netif_set_dns_info(arg, ESP_NETIF_DNS_MAIN, &dnsinfo) != ESP_OK){
            ESP_LOGW(TAG, "Failed to configure DNS server!");
        }
        #ifdef HOSTNAME
            if(esp_netif_set_hostname(arg, HOSTNAME) != ESP_OK){
                ESP_LOGW(TAG, "Failed to set Hostname!");
            }
        #endif
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifiInit(){
    initNVS();
    // Start TCP/IP
    esp_netif_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Create netif
    esp_netif_t *netif = NULL;
    netif = esp_netif_create_default_wifi_sta();
    assert(netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, netif));
    //ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifiConfig = {
        .sta = {
            .ssid = WAP_HOME_SSID,
            .password = WAP_HOME_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };

    ESP_LOGI(TAG, "Connecting via SSID:\t%s", wifiConfig.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());
    esp_wifi_connect();

    ESP_LOGI(TAG,"Wifi statred successfully!");
    return ESP_OK;
}
