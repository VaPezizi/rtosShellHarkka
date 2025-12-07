#pragma once
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
  const char * command;
  void (*function)(void * params);
  const char * description;
} Command;

typedef struct {
  QueueHandle_t * inputQueue;
  QueueHandle_t * outputQueue;
  TaskHandle_t lcdTaskHandle;
  TaskHandle_t fileSystemTaskHandle;
  //TaskHandle_t taskHandles;
  char argBuf[64];
  QueueHandle_t * fsOutputQueue;
} ShellTaskParams;

void helpTask(void * params);
void clearTask(void * params);
void lsTask(void * params);
void catTask(void * params);
void pwdTask(void * params);
void cdTask(void * params);
void touchTask(void * params);
void mkdirTask(void * params);
void rmTask(void * params);
void rmdirTask(void * params);
void tasksTask(void * params);
void killTask(void * params);
void rebootTask(void * params);
void writeTask(void * params);
void appendTask(void * params);

void t_return(TaskHandle_t shellTaskHandle, int returnCode);

void shellTask(void * params);
void handleCommand(const char * command, size_t length, TaskHandle_t * currentTask = NULL);

extern const Command commands[];