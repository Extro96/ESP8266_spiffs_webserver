#include "customSpiffs.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "customSpiffs";

void getSpiffsPath(char *fileName, char **path){
    *path = (char*)calloc(7+strlen(fileName), sizeof(char));
    sprintf(*path, "/spiffs%s",fileName);
}

esp_spiffs_err_t getFileContent(char file_name[], char **buffer){
    /* declare a file pointer */
    FILE    *infile;
    char    *path;

    /* get the path relative to the spiffs */
    getSpiffsPath(file_name, &path);

    /* open an existing file for reading */
    infile = fopen(path, "r");
    
    /* free the memory */
    free(path);

    /* quit if the file does not exist */
    if(infile == NULL){
        ESP_LOGI(TAG, "The file : %s doesn't exist", file_name);
        return ESP_SPIFFS_NO_FILE;
    }
    /* Get the number of bytes */
    fseek(infile, 0L, SEEK_END);
    
    /* reset the file position indicator to 
    the beginning of the file */
    fseek(infile, 0L, SEEK_SET);	
    
    /* grab sufficient memory for the 
    buffer to hold the text */
    *buffer = (char*)calloc(getFileSize(file_name), sizeof(char));
    
    /* memory error */
    if(*buffer == NULL){
        ESP_LOGI(TAG, "Memory error");
        return ESP_SPIFSS_MEMORY_ERROR;
    }

    /* copy all the text into the buffer */
    fread(*buffer, sizeof(char), getFileSize(file_name), infile);
    fclose(infile);
    return ESP_SPIFFS_OK;
}

long int getFileSize(char file_name[])
{
    char    *path;

    /* get the path relative to the spiffs */
    getSpiffsPath(file_name, &path);

    // opening the file in read mode
    FILE* fp = fopen(path, "r");

    free(path);
  
    // checking if the file exist or not
    if (fp == NULL) {
        ESP_LOGI(TAG, "The file : %s doesn't exist", file_name);
        return -1;
    }
  
    fseek(fp, 0L, SEEK_END);
  
    // calculating the size of the file
    long int res = ftell(fp);
  
    // closing the file
    fclose(fp);
  
    return res;
}

void startSpiffs(){
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void stopSpiffs(){
    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}