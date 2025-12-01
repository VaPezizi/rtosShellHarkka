#include "shell.h"
#include "serial.h"
#include "lcd.h"
#include "filesys.h"
//TODO:
/*
  - Implement command functions
  - Implement debug task for handling Serial output safely
  - Clean up the code, and divide to appropriate functions
  - Change started tasks to use the second core
  - Fix bugs with task notifications and input handling
*/

TaskHandle_t shellTaskHandle = NULL;

void output_to_que(const char * msg, size_t len, ShellTaskParams * params)
{

  for (size_t i = 0; i < len; i++)
  {
    if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100)))
    {
      if (xQueueSend(*params->outputQueue, &msg[i], pdMS_TO_TICKS(100)) != pdTRUE)
      {
        //Serial.println("Shell Task: Failed to send char to outputQueue");
      }
      xSemaphoreGive(outputQueueMutex);
    }

  }
}

//Not yet used
void t_return(TaskHandle_t shellTaskHandle, int returnCode)
{
  Serial.printf("shellTaksHandle: %p\n", shellTaskHandle);
  Serial.println("Shell Task: Notifying shell task of exit.");
  xTaskNotify(shellTaskHandle, returnCode, eNoAction);
  vTaskDelete(NULL);
}

void handleCommand(const char * command, size_t length, TaskHandle_t * currentTask, ShellTaskParams * shellParams = NULL)
{
  Serial.printf("Shell Task (Handle cmd): Handling command: %s\n", command);
  
  char cmd[32] = {0};
  char args[32] = {0};

  for (size_t i = 0; i < length; i++)
  {
    //Serial.printf("%c", command[i]);
    if(command[i] == ' ')
    {
      strncpy(cmd, command, i);
      cmd[i] = '\0';
      strncpy(args, command + i + 1, length - i - 1);
      break;
    }
  }
  if(args == NULL || strlen(args) == 0)
  {
    strncpy(cmd, command, length);
    //cmd[length] = '\0';
  }
  for(size_t i = 0; i < sizeof(commands) / sizeof(Command); i++)
  {
    if(strcmp(cmd, commands[i].command) == 0)
    {
      //Serial.printf("Shell Task (Handle cmd): Found command: %s\n", commands[i].command);
      if(commands[i].function != NULL)
      {
        //commands[i].function((void *)args);
        //Serial.printf("Shell Task (Handle cmd): Launching command %p as a new task.\n", commands[i].function);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        xTaskCreatePinnedToCore(
          (TaskFunction_t)commands[i].function,
          (const char *)commands[i].command,
          2048,
          (void *)shellParams,
          (UBaseType_t)1,
          currentTask,
          1
        );
        Serial.printf("Shell Task (Handle cmd): Launched command %s as a new task %p.\n", commands[i].command, *currentTask);
        //Serial.printf("Handle cmd, started task state: %", vTaskGetInfo);
      }else
      {
        Serial.printf("Shell Task (Handle cmd): Command %s not implemented yet.\n", commands[i].command);

      }
      //return;
    }
  }/*
Serial.printf("Shell Task (Handle cmd): Handling command: %s\n", command);
  char cmd[32] = {0};
  char args[32] = {0};
  // ...existing code...
  for(size_t i = 0; i < sizeof(commands) / sizeof(Command); i++)
  {
    if(strcmp(cmd, commands[i].command) == 0)
    {
      Serial.printf("Shell Task (Handle cmd): Found command: %s\n", commands[i].command);
      if(commands[i].function != NULL)
      {
        // REMOVE direct synchronous call that deleted shell task:
        // commands[i].function((void *)args);

        Serial.printf("Shell Task (Handle cmd): Launching command %p as a new task.\n", commands[i].function);
        xTaskCreatePinnedToCore(
          (TaskFunction_t)commands[i].function,
          (const char *)commands[i].command,
          2048,
          (void *)shellParams,          // Pass proper params object
          (UBaseType_t)1,
          currentTask,
          1
        );
      } else {
        Serial.printf("Shell Task (Handle cmd): Command %s not implemented yet.\n", commands[i].command);
      }
      break;
    }
  }

  Serial.printf("Shell Task (Handle cmd): Handling command: %s\n", command);
  char cmd[32] = {0};
  char args[32] = {0};
  // ...existing code...
  for(size_t i = 0; i < sizeof(commands) / sizeof(Command); i++)
  {
    if(strcmp(cmd, commands[i].command) == 0)
    {
      Serial.printf("Shell Task (Handle cmd): Found command: %s\n", commands[i].command);
      if(commands[i].function != NULL)
      {
        // REMOVE direct synchronous call that deleted shell task:
        // commands[i].function((void *)args);

        Serial.printf("Shell Task (Handle cmd): Launching command %p as a new task.\n", commands[i].function);
        xTaskCreatePinnedToCore(
          (TaskFunction_t)commands[i].function,
          (const char *)commands[i].command,
          2048,
          (void *)shellParams,          // Pass proper params object
          (UBaseType_t)1,
          currentTask,
          1
        );
      } else {
        Serial.printf("Shell Task (Handle cmd): Command %s not implemented yet.\n", commands[i].command);
      }
      break;
    }
  }*/
}



void helpTask(void * params)
{
  char help[32] = "Available commands:\n";
  //if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100)))
  
  if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100)))
  {
    for (size_t j = 0; j < strlen(help); j++)
    {
      if (xQueueSend(*((ShellTaskParams *)params)->outputQueue, &help[j], pdMS_TO_TICKS(100)) != pdTRUE)
      {
        //Serial.println("Shell Task: Failed to send help char to outputQueue");
      }
    }
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    for (size_t i = 0; i < sizeof(commands) / sizeof(Command); i++)
    {
      
        const char * cmd = commands[i].command;

        for (size_t j = 0; j < strlen(help); j++)
        {
          if (xQueueSend(*((ShellTaskParams *)params)->outputQueue, &cmd[j], pdMS_TO_TICKS(100)) != pdTRUE)
          {
            Serial.println("Shell Task: Failed to send help char to outputQueue");
          }
        }
        if(xQueueSend(*((ShellTaskParams *)params)->outputQueue, "\n", pdMS_TO_TICKS(100)) != pdTRUE)
        {
          Serial.println("Shell Task: Failed to send help char to outputQueue");
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    xSemaphoreGive(outputQueueMutex);
  }else{
    Serial.println("Shell Task: Failed to take outputQueueMutex in helpTask");
  }
  t_return(shellTaskHandle, 0);
}
void clearTask(void * params)
{
  Serial.println("Clear Task executed.");
  // Implement clear functionality if needed
  t_return(shellTaskHandle, 0);
}
void lsTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *) params;

  vTaskDelay(500 / portTICK_PERIOD_MS);
  FileSystemRequest req = {};
  req.operation   = FS_OP_LIST;
  strncpy(req.path, "/", sizeof(req.path)-1);
  req.levels      = 1;
  req.outputQueue = *sp->outputQueue;          // reuse shell’s output queue
  req.notifyTask  = xTaskGetCurrentTaskHandle();

  xQueueSend(fsInQueue, &req, portMAX_DELAY);

  // Wait for completion marker (optional)
  for(;;){
      char ch;
      if(xQueueReceive(req.outputQueue, &ch, pdMS_TO_TICKS(1000)) == pdTRUE)
      {
          if(ch == '\x04') break; // EOT
          // forward to LCD or serial as your pipeline requires
          output_to_que(&ch, 1, sp);
      } else
       {
          break;
      }
  }
  t_return(shellTaskHandle, 0);
  
}

void shellTask(void * params)
{
  //Perhaps we add a list of taskhandels that can be lauchhed from the shell

  shellTaskHandle = xTaskGetCurrentTaskHandle();

  TaskHandle_t currentTask = NULL;
  ShellTaskParams * shellParams = (ShellTaskParams *) params;
  

  char inputBuffer[64] = {0};
  uint8_t bufferIndex = 0;
  char lastChar = 0;
  
  while(1)
  {


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

          output_to_que(&ch, 1, shellParams);
          
        }
        
        
        else  //If in a launched task
        {
          //If in a launched task, we can implement task-specific input handling here
          //Serial.printf("Shell Task: Input directed to task %p: %c\n", currentTask, ch);
          
        }


      }
      else
      {
        xSemaphoreGive(inputQueueMutex);
        if(currentTask != NULL){
          if(xTaskNotifyWait(0, 0, (uint32_t*)&ch, pdMS_TO_TICKS(100)) == pdTRUE)
          {
            //Serial.println("Shell Task: Failed to notify current task with input char");
            char msg[32] = "Task returned with code: ";
            itoa((int)ch, msg + strlen(msg), 10);
            
            Serial.printf("Shell Task: notify returned with %d\n", (int)ch);
            
            /*if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(100)))
            {
              for(size_t i = 0; i < strlen(msg); i++)
              {
                if (xQueueSend(*shellParams->outputQueue, &msg[i], pdMS_TO_TICKS(100)) != pdTRUE)
                {
                  Serial.println("Shell Task: Failed to send char to outputQueue");
                }
                vTaskDelay(50 / portTICK_PERIOD_MS);
              }
              xSemaphoreGive(outputQueueMutex);
            }*/
            output_to_que(msg, strlen(msg), shellParams);
            currentTask = NULL; //Return to shell mode
          }
        }


        
          //Serial.println("Shell Task: No char received from queue in shell mode");
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