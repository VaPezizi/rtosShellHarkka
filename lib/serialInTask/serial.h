#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

extern volatile SemaphoreHandle_t inputQueueMutex;

typedef struct {
  TaskHandle_t shellTaskHandle;
  QueueHandle_t * inputQueue;
} SerialInputTaskParams;

void serialInputTask(void * params);