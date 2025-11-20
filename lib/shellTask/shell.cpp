#include "shell.h"
#include "serial.h"

void handleCommand(const char * command, size_t length){
  Serial.printf("Shell Task: Handling command: %s\n", command);
  // Add command handling logic here
}

void shellTask(void * params){
  TaskHandle_t currentTask;
  ShellTaskParams * shellParams = (ShellTaskParams *) params;
  

  char inputBuffer[64] = {0};
  uint8_t bufferIndex = 0;
  char lastChar = 0;
  
  while(1){

    //Wait for notification from serial input task
    char ch;

    if (xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
      if (xQueueReceive(*shellParams->inputQueue, &ch, pdMS_TO_TICKS(100)) == pdTRUE){
        Serial.printf("Shell Task: Received char from queue: %c\n", ch);

        if (ch == '\b' || ch == 127){ // Handle backspace
          if (bufferIndex > 0){
            bufferIndex--;
            inputBuffer[bufferIndex] = '\0';
          }

          // echo to lcd
          if (xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
            char backspaceChar = '\b';
            if (xQueueSend(*shellParams->stdQueue, &backspaceChar, pdMS_TO_TICKS(100)) != pdTRUE){
              Serial.println("Shell Task: Failed to send backspace char to stdQueue");
            }
            xSemaphoreGive(inputQueueMutex);
          }
          continue; // Skip further processing for backspace
        }
        if (currentTask == NULL){

          if (ch == '\n' || ch == '\r'){
            inputBuffer[bufferIndex] = '\0'; // Null-terminate the string
            Serial.printf("Shell Task: Complete input received: %s\n", inputBuffer);
            bufferIndex = 0;                             // Reset buffer index for next input
            memset(inputBuffer, 0, sizeof(inputBuffer)); // Clear the input buffer
            ch = '\x1b';                                 // Sending clear screen command to LCD task
          }

          // Echo back to stdQueue
          if (xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
            if (xQueueSend(*shellParams->stdQueue, &ch, pdMS_TO_TICKS(100)) != pdTRUE){
              Serial.println("Shell Task: Failed to send char to stdQueue");
            }
            xSemaphoreGive(inputQueueMutex);
          }

          // Build input buffer
          else{
            if (bufferIndex < sizeof(inputBuffer) - 1){
              inputBuffer[bufferIndex++] = ch;
            }
          }
        }
        xSemaphoreGive(inputQueueMutex);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    }
    
  }
}