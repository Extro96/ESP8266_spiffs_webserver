#ifndef _CUSTOM_SPIFFS_H_
#define _CUSTOM_SPIFFS_H_

#include "esp_spiffs.h"

typedef enum{
    /* Everything went fine. */
    ESP_SPIFFS_OK = 0,
    /* File was not found. */
    ESP_SPIFFS_NO_FILE,
    /* Memory error. */
    ESP_SPIFSS_MEMORY_ERROR,
    /* File has no extension */
    ESP_SPIFFS_NO_FILE_EXT
} esp_spiffs_err_t;

void startSpiffs(void);

void stopSpiffs(void);

void getSpiffsPath(char *path, char **buffer);

long int getFileSize(char file_name[]);

esp_spiffs_err_t getFileContent(char file_name[], char **buffer);

char* getContentType(char file_name[]);

#endif /* ! _CUSTOM_SPIFFS_H_ */