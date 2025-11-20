#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
  QueueHandle_t * inputQueue;
  QueueHandle_t * stdQueue;
  TaskHandle_t lcdTaskHandle;
  TaskHandle_t fileSystemTaskHandle;
} ShellTaskParams;

void shellTask(void * params);
void handleCommand(const char * command, size_t length);


