#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/queue.h"
//#include <SD.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD_MMC.h>


#define FORMAT_LITTLEFS_IF_FAILED true //Set to true to format LittleFS on startup if mount fails


//Some internal pins used for the sd card in this chip (ESP32 Wrover)
#define SD_MMC_CMD 15 //Please do not modify it.
#define SD_MMC_CLK 14 //Please do not modify it. 
#define SD_MMC_D0  2  //Please do not modify it.

typedef struct 
{
    TaskHandle_t shellTaskHandle;
    //QueueHandle_t requestQueue;
}FileSystemTaskParams;

typedef struct 
{
    const char *currentPath;
    
} FileSystemState;

typedef enum
{
    FS_OP_LIST,
    FS_OP_READ,
    FS_OP_WRITE,
    FS_OP_DELETE,
    FS_OP_MKDIR,
    FS_OP_RMDIR,
    FS_OP_RENAME
} FsOp;

typedef struct 
{
    FsOp operation;
    char path[128];
    uint8_t levels; //Used for rename operations
    const char * data; //Used for write operations
    QueueHandle_t outputQueue;    // where to stream textual results
    TaskHandle_t notifyTask;      // task to notify when done (optional)
} FileSystemRequest;

typedef struct
{
    char * data;
    size_t length;
} FileReadResult;

extern SemaphoreHandle_t fsMutex;
extern QueueHandle_t fsInQueue;
//rn QueueHandle_t fsOutQueue;

// https://randomnerdtutorials.com/esp32-write-data-littlefs-arduino/
int listDir(fs::FS &fs, const char * dirname, uint8_t levels,FileSystemRequest * fsReq);
int createDir(fs::FS &fs, const char * path, FileSystemRequest * fsReq);
int removeDir(fs::FS &fs, const char * path, FileSystemRequest * fsReq);
int readFile(fs::FS &fs, const char * path, FileSystemRequest * fsReq);
int writeFile(fs::FS &fs, const char * path, const char * message, FileSystemRequest * fsReq);
int appendFile(fs::FS &fs, const char * path, const char * message, FileSystemRequest * fsReq);
int renameFile(fs::FS &fs, const char * path1, const char * path2, FileSystemRequest * fsReq);
int deleteFile(fs::FS &fs, const char * path, FileSystemRequest * fsReq);
void testFileIO(fs::FS &fs, const char * path, FileSystemRequest * fsReq);

void fileSystemTask(void * params);