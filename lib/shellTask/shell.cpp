#include "shell.h"
#include "serial.h"
#include "lcd.h"

inline void handleCommand(const char * command, size_t length, TaskHandle_t * currentTask = NULL, ShellTaskParams * shellParams = NULL){
  Serial.printf("Shell Task (Handle cmd): Handling command: %s\n", command);
  
  char cmd[32] = {0};
  char args[32] = {0};

  for (size_t i = 0; i < length; i++){
    //Serial.printf("%c", command[i]);
    if(command[i] == ' '){
      strncpy(cmd, command, i);
      cmd[i] = '\0';
      strncpy(args, command + i + 1, length - i - 1);
      break;
    }
  }
  if(args == NULL || strlen(args) == 0){
    strncpy(cmd, command, length);
    //cmd[length] = '\0';
  }
  for(size_t i = 0; i < sizeof(commands) / sizeof(Command); i++){
    if(strcmp(cmd, commands[i].command) == 0){
      Serial.printf("Shell Task (Handle cmd): Found command: %s\n", commands[i].command);
      if(commands[i].function != NULL){
        commands[i].function((void *)args);
        Serial.printf("Shell Task (Handle cmd): Launching command %p as a new task.\n", commands[i].function);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        xTaskCreatePinnedToCore(
          (TaskFunction_t)commands[i].function,
          (const char *)commands[i].command,
          2048,
          (void *)args,
          (UBaseType_t)1,
          currentTask,
          2
        );
      }else{
        Serial.printf("Shell Task (Handle cmd): Command %s not implemented yet.\n", commands[i].command);

      }
      //return;
    }
  }
}

void helpTask(void * params){
  char help[32] = "Available commands:\n";
  //if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100)))
  
  if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100))){
    for (size_t j = 0; j < strlen(help); j++){
      if (xQueueSend(*((ShellTaskParams *)params)->outputQueue, &help[j], pdMS_TO_TICKS(100)) != pdTRUE){
        Serial.println("Shell Task: Failed to send help char to outputQueue");
      }
    }
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    for (size_t i = 0; i < sizeof(commands) / sizeof(Command); i++)
    {
      
        const char * cmd = commands[i].command;

        for (size_t j = 0; j < strlen(help); j++){
          if (xQueueSend(*((ShellTaskParams *)params)->outputQueue, &cmd[j], pdMS_TO_TICKS(100)) != pdTRUE){
            Serial.println("Shell Task: Failed to send help char to outputQueue");
          }
        }
        if(xQueueSend(*((ShellTaskParams *)params)->outputQueue, "\n", pdMS_TO_TICKS(100)) != pdTRUE){
          Serial.println("Shell Task: Failed to send help char to outputQueue");
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    xSemaphoreGive(outputQueueMutex);
  }else{
    Serial.println("Shell Task: Failed to take outputQueueMutex in helpTask");
  }
  vTaskDelete(NULL);
}
void clearTask(void * params){
  Serial.println("Shell Task: Clear command executed.");
  // Implement clear functionality if needed
  vTaskDelete(NULL);
}
void lsTask(void * params){
  Serial.println("Shell Task: ls command executed.");
  // Implement ls functionality if needed
  vTaskDelay(500 / portTICK_PERIOD_MS);
  vTaskDelete(NULL);
}

void shellTask(void * params){
  //Perhaps we add a list of taskhandels that can be lauchhed from the shell



  TaskHandle_t currentTask = NULL;
  ShellTaskParams * shellParams = (ShellTaskParams *) params;
  

  char inputBuffer[64] = {0};
  uint8_t bufferIndex = 0;
  char lastChar = 0;
  
  while(1){


    char ch;
    /*if(inputQueueMutex == NULL  || outputQueueMutex == NULL){
      Serial.println("Shell Task: AAA is NULL!");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }*/
    if (xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      if (xQueueReceive(*shellParams->inputQueue, &ch, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        Serial.printf("Shell Task: Received char from queue: %c\n", ch);
        xSemaphoreGive(inputQueueMutex);
        /*
        }*/
        //If in shell mode
        if (currentTask == NULL)
        {
          if (ch == '\b' || ch == 127)
          { // Handle backspace
            if (bufferIndex > 0)
            {
              bufferIndex--;
              inputBuffer[bufferIndex] = '\0';
            }
          }
          else if (ch == '\n' || ch == '\r')
          {
            inputBuffer[bufferIndex] = '\0'; // Null-terminate the string
            handleCommand(inputBuffer, bufferIndex, &currentTask, shellParams);
            Serial.printf("Shell Task: Complete input received: %s\n", inputBuffer);
            bufferIndex = 0;                             // Reset buffer index for next input
            memset(inputBuffer, 0, sizeof(inputBuffer)); // Clear the input buffer
            ch = '\x1b';                                 // Sending clear screen command to LCD task
          }          
          else
          {
            if (bufferIndex < sizeof(inputBuffer) - 1)
            {
              inputBuffer[bufferIndex++] = ch;
            }
          }

          if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
            // Send backspace to LCD
            if (xQueueSend(*shellParams->outputQueue, &ch, pdMS_TO_TICKS(100)) != pdTRUE)
            {
              Serial.println("Shell Task: Failed to send char to outputQueue");
            }
            xSemaphoreGive(outputQueueMutex);
            //continue;
          }else
          {
            Serial.println("Shell Task: Failed to take outputQueueMutex");
          }
          

          // Echo back to stdQueue
          /*if (xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
            if (xQueueSend(*shellParams->outputQueue, &ch, pdMS_TO_TICKS(100)) != pdTRUE){
              Serial.println("Shell Task: Failed to send char to stdQueue");
            }
            xSemaphoreGive(inputQueueMutex);
            continue;
          }*/

          // Build input buffer
        }
        
        
        else  //If in a launched task
        {
          //If in a launched task, we can implement task-specific input handling here
          //xTaskNotify(currentTask, (uint32_t)ch, eSetValueWithOverwrite);
          Serial.printf("Shell Task: Input directed to task %p: %c\n", currentTask, ch);
          if(currentTask == NULL)
          {
            Serial.println("Shell Task: Current task is NULL!");
          }
          //xTaskNotify(currentTask, (uint32_t)ch, eSetValueWithOverwrite);

        }


      }else
      {
        xSemaphoreGive(inputQueueMutex);
        //Serial.println("Shell Task: No char received from queue");
      }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);

    }else
    {
      //xSemaphoreGetMutexHolder(inputQueueMutex);
      Serial.println("Shell Task: Failed to take inputQueueMutex");
      Serial.printf("Mutex held by task: %p\n", xSemaphoreGetMutexHolder(inputQueueMutex));
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
  }
}