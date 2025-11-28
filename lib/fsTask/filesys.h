#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/queue.h"
//#include <SD.h>

#include "FS.h"
#include "LittleFS.h"
#include "SD_MMC.h"

#define FORMAT_LITTLEFS_IF_FAILED true //Set to true to format LittleFS on startup if mount fails


//Some internal pins used for the sd card in this chip (ESP32 Wrover)
#define SD_MMC_CMD 15 //Please do not modify it.
#define SD_MMC_CLK 14 //Please do not modify it. 
#define SD_MMC_D0  2  //Please do not modify it.

typedef struct {
    TaskHandle_t shellTaskHandle;
    const char * basePath;
}FileSystemTaskParams;

typedef struct {
    const char *currentPath;
    
} FileSystemState;

static SemaphoreHandle_t fileSystemMutex;
static QueueHandle_t fileSystemQueue;

// https://randomnerdtutorials.com/esp32-write-data-littlefs-arduino/
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);

void fileSystemTask(void * params);