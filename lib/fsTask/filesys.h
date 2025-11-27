#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/queue.h"
//#include <SD.h>

#include "FS.h"
#include "LittleFS.h"
#include "SD_MMC.h"


//Some internal pins used for the sd card in this chip (ESP32 Wrover)
#define SD_MMC_CMD 15 //Please do not modify it.
#define SD_MMC_CLK 14 //Please do not modify it. 
#define SD_MMC_D0  2  //Please do not modify it.

typedef struct {
    TaskHandle_t shellTaskHandle;
    const char * basePath;
}FileSystemTaskParams;

void fileSystemTask(void * params);