#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

extern volatile SemaphoreHandle_t outputQueueMutex; // Set the LCD I2C address

void lcdTask(void * params);

typedef struct {
  QueueHandle_t * inputQueue;
  uint8_t lcdCols;
  uint8_t lcdRows;
  uint8_t address;
} LCDTaskParams;