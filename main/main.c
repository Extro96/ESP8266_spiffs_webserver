/* SPIFFS filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h> 
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "customSpiffs.h"
#include "customSocket.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <esp_http_server.h>

static const char *TAG = "main";



/* An HTTP GET handler */
esp_err_t hello_get_handler(httpd_req_t *req)
{
    /* on va mettre le fichier la dedans */
    char    *monBuffer;
    esp_spiffs_err_t result;

    result = getFileContent("/index.html", &monBuffer);

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    if(result == ESP_SPIFFS_OK){
        httpd_resp_send(req, monBuffer,getFileSize("/index.html"));
    }
    ESP_LOGI(TAG, "uri demandee : %s", req->uri);
    free(monBuffer);

    return ESP_OK;
}

/* An HTTP GET handler */
esp_err_t spiffs_get_handler(httpd_req_t *req)
{
    char    *monBuffer;
    esp_spiffs_err_t result;

    result = getFileContent((char *)req->uri, &monBuffer);

    ESP_LOGI(TAG, "uri demandee : %s", req->uri);

    if(result == ESP_SPIFFS_OK){                                            // If the uri match with a file
        char* contentType = getContentType((char *)req->uri);
        httpd_resp_set_type(req, contentType);
        httpd_resp_send(req, monBuffer, getFileSize((char *)req->uri));
    }else if(strcmp(req->uri,"/") == 0){                                    // If no uri, send index.html
        result = getFileContent("/index.html", &monBuffer);
        if(result == ESP_SPIFFS_OK){
            httpd_resp_send(req, monBuffer,getFileSize("/index.html"));
        }
    }else{                                                                  // If the resqueted uri doesn't refer to a file
        // TODO renvoyer une page d'erreur 404
        const char* resp_str = (const char*) req->user_ctx;
        httpd_resp_send(req, resp_str, strlen(resp_str));
    }

    /* free the memory */
    free(monBuffer);

    return ESP_OK;
}

httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

httpd_uri_t spiffs = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = spiffs_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &spiffs);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static httpd_handle_t server = NULL;

static void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void app_main(void)
{
    startSpiffs();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    ESP_ERROR_CHECK(example_connect());

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    startSocket();

    server = start_webserver();

    DIR *d;
    struct dirent *dir;
    d = opendir("/spiffs");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            ESP_LOGI(TAG, "Read from spiffs: '%s'", dir->d_name);
        }
        closedir(d);
    }else{
        ESP_LOGI(TAG, "No spiffs files found");
    }
}