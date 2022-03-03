# ESP8266 RC CAR

***This project is intended to be a formation on the ESP8266_RTOS_SDK.***

In this project, we will flash the esp8266 and install it on an old rc car. It will provide an IHM over the wifi to control it through a phone or a computer.
We will see how to flash the spiffs, control pwm, I/O, use FreeRTOS, websockets, web pages interractions.

## Getting started with the ESP8266_RTOS_SDK
You need to install and configure the ESP8266_RTOS_SDK before continuing, everything is explain on their [GitHub page](https://github.com/espressif/ESP8266_RTOS_SDK/).

## Upload files to the spiffs
***This solution is only for Cmake (didn't got it work on make)***

In order to upload files in the spiffs, you first need to modify your ESP8266_RTOS_SDK to get it working.
We need to add some tools from the [ESP-IDF V4.0](https://github.com/espressif/esp-idf/tree/release/v4.0) and get it working on the ESP8266_RTOS_SDK.
- Copy sp-idf-4.0.3/components/spiffs/spiffsgen.py to ${IDF_PATH}/components/spiffs/spiffsgen.py
- Copy sp-idf-4.0.3/components/spiffs/project_include.cmake to ${IDF_PATH}/components/spiffs/project_include.cmake
- Modify ${IDF_PATH}/components/spiffs/project_include.cmake by adding option --aligned-obj-ix-tables and replace ${use_magic_len} by --no-magic-len (otherwise file content is in accessible):

``` cmake
add_custom_target(spiffs_${partition}_bin ALL
    COMMAND ${spiffsgen_py} ${size} ${base_dir_full_path} ${image_file}
    --page-size=${CONFIG_SPIFFS_PAGE_SIZE}
    --obj-name-len=${CONFIG_SPIFFS_OBJ_NAME_LEN}
    --meta-len=${CONFIG_SPIFFS_META_LENGTH}
    --aligned-obj-ix-tables
    ${use_magic}
    --no-magic-len
    DEPENDS ${arg_DEPENDS}
    )
```

You can now build your binary (storage.bin) containing everything that was in the folder $(PROJECT_PATH)/main/data with 'make all' and flash it with 'make flash' (with CMake)
> Work with the CMake extension on Visual Studio Code, `target : all` then `target : flash`

### Prepare the spiffs for your own project

- Modify $(PROJECT_PATH)/main/CMakeLists.txt to add the instruction to build and flash your files that you want in the spiffs.
```
spiffs_create_partition_image(storage data FLASH_IN_PROJECT)
```
> *spiffs_create_partition_image(<partition> <base_dir> [FLASH_IN_PROJECT] [DEPENDS dep dep dep...])*

- Create your own partition.csv like shown in ${IDF_PATH}/examples/storage/spiffs
- Configure your project by typing `make menuconfig` at the root of your project.
  - Serial flasher config -> Configure your flash size
  - Partition Table -> Select your own partition.csv
  - Component config -> SPIFFS Configuration : Disable "SPIFFS Filesystem Length Magic"

#### Sources that helped me find the solution :
- [SPIFFS images - What is different between ESP32 and ESP8266?](https://www.esp32.com/viewtopic.php?t=21955)  
- [ESP-IDF SPIFFS can't find files on ESP32?](https://esp32.com/viewtopic.php?t=7413)
- [esp-idf SPIFFS Filesystem](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html)


## Getting a webserver with the spiffs
I didn't find a function that would be triggered if the user ask for an unknown page (error 404). So i had to modify the sdk to make my own.
In that way, I can browse the spiffs to check if the desired file exist and send the required files.

My modification work as follow : 
You have to create a handler that head to `"/*"`
Once that handler is initialized, everytime an user ask for a page that isn't managed, he will get redirected to your handling function.
Once in that handling function, I browse the spiffs with the given path to check if there's a file. If not, I send a 404 error to the user.

modified function in ${IDF_PATH}/components/esp_http_server/src/httpd_uri.c

``` c
/* Alternate implmentation of httpd_find_uri_handler()
 * which takes a uri_len field. This is useful when the URI
 * string contains extra parameters that are not to be included
 * while matching with the registered URI_handler strings
 */
static httpd_uri_t* httpd_find_uri_handler2(httpd_err_resp_t *err,
                                            struct httpd_data *hd,
                                            const char *uri, size_t uri_len,
                                            httpd_method_t method)
{
    *err = 0;
    for (int i = 0; i < hd->config.max_uri_handlers; i++) {
        if (hd->hd_calls[i]) {
            ESP_LOGD(TAG, LOG_FMT("[%d] = %s"), i, hd->hd_calls[i]->uri);
            if ((strlen(hd->hd_calls[i]->uri) == uri_len) &&            // First match uri length
                (strncmp(hd->hd_calls[i]->uri, uri, uri_len) == 0))  {  // Then match uri strings
                if (hd->hd_calls[i]->method == method)  {               // Finally match methods
                    return hd->hd_calls[i];
                }
                /* URI found but method not allowed.
                 * If URI IS found later then this
                 * error is to be neglected */
                *err = HTTPD_405_METHOD_NOT_ALLOWED;
            }
            if(strcmp(hd->hd_calls[i]->uri, "/*") == 0){ // If the "/*" uri is define
                if(hd->hd_calls[i]->method == method){
                    return hd->hd_calls[i];
                }
                *err = HTTPD_405_METHOD_NOT_ALLOWED;    // The method is not the good one
            }
        }
    }
    if (*err == 0) {
        *err = HTTPD_404_NOT_FOUND;
    }
    return NULL;
}
```

- [ ] Adding an ifdef around the added function in order to disable the modification if the spiffs isn't defined/used.
- [ ] Adding the 404 response if no file is found.
- [x] Make the getFile from spiffs more robust.
- [x] Adding and sending mime type.
- [x] Redirect to `/index.html` if no uri referred.
