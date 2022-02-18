#ifndef _CUSTOM_SPIFFS_H_
#define _CUSTOM_SPIFFS_H_

#include "esp_spiffs.h"

void startSpiffs(void);

void stopSpiffs(void);

void getSpiffsPath(char *path, char **buffer);

long int getFileSize(char file_name[]);

void getFileContent(char file_name[], char **buffer);

#endif /* ! _CUSTOM_SPIFFS_H_ */