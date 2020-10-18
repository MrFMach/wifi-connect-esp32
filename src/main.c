#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"

#define SSID "wifi ssid"
#define SSID_PASS "wifi password"

#define HTTP_SERVER_IP "93.184.216.34" // example.com
#define HTTP_SERVER_PORT 80

static const char *REQUEST_MESSAGE = "GET / HTTP/1.0\r\n"
                                     "Host: example.com\r\n"
                                     "User-Agent: ESP32 WiFi HTTP Client\r\n"
                                     "\r\n";

void requestHTTP(void);

esp_err_t event_handler(void *ctx, system_event_t *event)
{

    if (event->event_id == SYSTEM_EVENT_STA_GOT_IP)
    {

        printf("\n --- IP access point received = %d.%d.%d.%d \n", IP2STR(&event->event_info.got_ip.ip_info.ip));
        requestHTTP();
    }
    return ESP_OK;
}

void requestHTTP()
{

    int sock, ret;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = PP_HTONS(HTTP_SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(HTTP_SERVER_IP);

    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("\n --- Sock < 0 - FATAL!!!\n");
        esp_restart();
    }

    ret = lwip_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0)
    {
        printf("\n --- Connect != 0 - FATAL!!!\n");
        esp_restart();
    }

    printf("\n --- Sending request HTTP...\n");
    lwip_send(sock, REQUEST_MESSAGE, strlen(REQUEST_MESSAGE), 0);

    char recvbuf[1024] = {0};

    while ((ret = lwip_recv(sock, recvbuf, 1024, 0)) > 0)
    {
        printf("\n --- Bytes received: %d\n", ret);
        recvbuf[ret] = 0;
        printf("\n --- Content received: %s\n", recvbuf);
    }

    lwip_close(sock);
}

static void init_nvs()
{
    /* Inicializa a partição nvs
     * Ela usada para armazenamento não-volátil, 
     * utilizada para armazenar calibração de dados. 
     * Ele também é usado para armazenar as configurações 
     * do WiFi, por exemplo e ela pode ser utilizada 
     * para outros dados de aplicação.
     */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void init_connect()
{
    ESP_ERROR_CHECK(esp_netif_init()); //Inicializa a pilha TCP/IP e retorna ESP_OK ou ESP_FAIL
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t sta_config = {
        .sta = {
            .ssid = SSID,
            .password = SSID_PASS,
            .bssid_set = 0}};

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void app_main()
{
    printf("\n --- init_nvs() --- \n\v");
    init_nvs();

    printf("\n --- init WiFi/HTTP --- \n\v");
    init_connect();
}
