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

void t_return(TaskHandle_t shellTaskHandle, int returnCode);

void shellTask(void * params);
void handleCommand(const char * command, size_t length, TaskHandle_t * currentTask = NULL);

static Command commands[] = {
  {"help",  helpTask,   "Show this help"},
  {"clear", clearTask,  "Clear screen"},
  {"ls",    lsTask,     "List directory"},
  {"cat",   catTask,    "Show file contents"},
  {"pwd",   pwdTask,    "Print working dir"},
  {"cd",    cdTask,     "Change directory"},
  {"touch", touchTask,  "Create empty file"},
  {"mkdir", mkdirTask,  "Create directory"},
  {"rm",    rmTask,     "Delete file"},
  {"rmdir", rmdirTask,  "Remove directory"},
  {"tasks", tasksTask,  "List RTOS tasks"},
  {"kill",  killTask,   "Kill task (stub)"},
  {"reboot",rebootTask, "Restart MCU"},
};