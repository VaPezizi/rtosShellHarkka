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

const Command commands[] ={
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
  {"write", writeTask, "Write data to a file"},
  {"append", appendTask, "Append data to a file"},
};

TaskHandle_t shellTaskHandle = NULL;
static char currentPath[128] = "/";

void output_to_que(const char * msg, size_t len, ShellTaskParams * params)
{

  for (size_t i = 0; i < len; i++)
  {
    if(xSemaphoreTake(outputQueueMutex, pdMS_TO_TICKS(500)))
    {
      if (uxQueueSpacesAvailable(*params->outputQueue) == 0)
      {
        //Serial.println("Shell Task: Output queue full, waiting...");
        xSemaphoreGive(outputQueueMutex);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        i--; // Retry sending this character
        continue;
      }
      if (xQueueSend(*params->outputQueue, &msg[i], pdMS_TO_TICKS(100)) != pdTRUE)
      {
        //Serial.println("Shell Task: Failed to send char to outputQueue");
      }
      xSemaphoreGive(outputQueueMutex);
    }

  }
}

void t_return(TaskHandle_t shellTaskHandle, int returnCode)
{
  Serial.printf("shellTaksHandle: %p\n", shellTaskHandle);
  Serial.println("Shell Task: Notifying shell task of exit.");
  xTaskNotify(shellTaskHandle, returnCode, eNoAction);
  vTaskDelete(NULL);
}


static void output_line(ShellTaskParams * sp, const char * s)
{
  output_to_que(s, strlen(s), sp);
  char nl = '\n';
  output_to_que(&nl, 1, sp);
}

static void buildPath(char * dst, size_t dstLen, const char * arg)
{
  if(arg[0] == '/' || currentPath[0] == '\0')
  {
    strncpy(dst, arg, dstLen-1);
  } else 
  {
    if(strcmp(currentPath, "/")==0)
    {
      snprintf(dst, dstLen, "/%s", arg);
    } else 
    {
      snprintf(dst, dstLen, "%s/%s", currentPath, arg);
    }
  }
  dst[dstLen-1] = 0;
}

static void fsStream(FsOp op, const char * path, const char * data, uint8_t levels, QueueHandle_t * outputQueue, ShellTaskParams * sp)
{
  FileSystemRequest req = {};
  req.operation   = op;
  strncpy(req.path, path, sizeof(req.path)-1);
  req.data        = data;
  req.levels      = levels;
  req.outputQueue = *outputQueue;
  req.notifyTask  = xTaskGetCurrentTaskHandle();
  xQueueSend(fsInQueue, &req, portMAX_DELAY);
  /*for(;;){
    char ch;
    if(xQueueReceive(req.outputQueue, &ch, pdMS_TO_TICKS(2000)) == pdTRUE){
      if(ch == '\x04') break;
      output_to_que(&ch, 1, sp);
    } else break;
  }*/
}
void helpTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  
  output_to_que("Available commands:", strlen("Available commands:"), sp);
  char nl='\n'; 
  
  output_to_que(&nl,1,sp);

  for(size_t i=0;i<sizeof(commands)/sizeof(Command);i++)
  {
    const Command &c = commands[i];
    output_to_que(c.command, strlen(c.command), sp);
    output_to_que(" - ", 3, sp);
    output_to_que(c.description, strlen(c.description), sp);
    output_to_que(&nl,1,sp);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  
  t_return(shellTaskHandle, 0);
}

void catTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  if(sp->argBuf[0] == 0)
  { 
    output_line(sp, "cat: missing file"); 
    t_return(shellTaskHandle, -1); 
    return; 
  }

  char path[128]; 
  buildPath(path, sizeof(path), sp->argBuf);
  fsStream(FS_OP_READ, path, NULL, 0, sp->outputQueue,sp);
  vTaskDelay(300 / portTICK_PERIOD_MS);
  t_return(shellTaskHandle, 0);
}

void pwdTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  output_line(sp, currentPath);
  t_return(shellTaskHandle, 0);
}

void cdTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  if(sp->argBuf[0] == 0)
  { 
    output_line(sp, "cd: missing path");
    t_return(shellTaskHandle,-1); 
  }
  
  int pathExists = 0;

  char newPath[128];
  buildPath(newPath, sizeof(newPath), sp->argBuf);

  size_t l = strlen(newPath);
  if(l>1 && newPath[l-1]=='/') newPath[l-1]=0;

  fsStream(FS_OP_LIST, newPath, NULL, 0, sp->fsOutputQueue,sp);
  if(xTaskNotifyWait(0, 0xFFFFFFFF,(uint32_t*)&pathExists, pdMS_TO_TICKS(2000)) == pdTRUE)
  {
    Serial.printf("cdTask: pathExists = %u\n", pathExists);
    if(pathExists == (uint32_t)-1)
    {
      output_line(sp, "cd: no such directory");
      t_return(shellTaskHandle, -1);
    }
  } else 
  {
    output_line(sp, "cd: timeout checking directory");
    t_return(shellTaskHandle, -1);
  }

  if(strcmp(sp->argBuf, "/")==0)
  {
    strcpy(currentPath, "/");
  } else 
  {
    strncpy(currentPath, newPath, sizeof(currentPath)-1);
  }
  // naive: just set path (validation could be added via FS_OP_LIST)
  /*
  if(strcmp(sp->argBuf, "/")==0){
    strcpy(currentPath, "/");
  } else {
    char newPath[128]; 
    buildPath(newPath, sizeof(newPath), sp->argBuf);
    // strip trailing slash
    size_t l = strlen(newPath);
    if(l>1 && newPath[l-1]=='/') newPath[l-1]=0;
    strncpy(currentPath, newPath, sizeof(currentPath)-1);
  }*/
  output_line(sp, currentPath);
  t_return(shellTaskHandle, 0);
}

void touchTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  if(sp->argBuf[0]==0)
  { 
    output_line(sp, "touch: missing file"); 
    t_return(shellTaskHandle,-1); 
    return; 
  }
  char path[128]; 
  buildPath(path,sizeof(path), sp->argBuf);
  fsStream(FS_OP_WRITE, path, "", 0, sp->outputQueue,sp);
  t_return(shellTaskHandle, 0);
}

void mkdirTask(void * params){
  ShellTaskParams * sp = (ShellTaskParams *)params;
  if(sp->argBuf[0]==0){ output_line(sp,"mkdir: missing dir"); t_return(shellTaskHandle,-1); return; }
  char path[128]; buildPath(path,sizeof(path), sp->argBuf);
  fsStream(FS_OP_MKDIR, path, NULL, 0, sp->outputQueue,sp);
  t_return(shellTaskHandle, 0);
}

void rmTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  if(sp->argBuf[0]==0){ output_line(sp,"rm: missing file"); t_return(shellTaskHandle,-1); return; }
  char path[128]; buildPath(path,sizeof(path), sp->argBuf);
  fsStream(FS_OP_DELETE, path, NULL, 0, sp->outputQueue,sp);
  t_return(shellTaskHandle, 0);
}

void rmdirTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  if(sp->argBuf[0]==0)
  { 
    output_line(sp,"rmdir: missing dir"); 
    t_return(shellTaskHandle,-1); return; 
  }

  char path[128]; 
  buildPath(path,sizeof(path), sp->argBuf);
  fsStream(FS_OP_RMDIR, path, NULL, 0, sp->outputQueue,sp);
  t_return(shellTaskHandle, 0);
}

void tasksTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
#if (configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS == 1)
  char buf[512]; vTaskList(buf);
  output_line(sp, "Name          State Prio Stack Num");
  output_to_que(buf, strlen(buf), sp);
#else
  output_line(sp, "Task listing not enabled (trace off)");
#endif
  t_return(shellTaskHandle, 0);
}

void killTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  output_line(sp, "kill: not implemented");
  t_return(shellTaskHandle, 0);
}

void rebootTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  output_line(sp, "Rebooting...");
  ESP.restart();
  t_return(shellTaskHandle, 0);
  
}

void writeTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  char * argBuf = sp->argBuf;
  char * spacePos = strchr(argBuf, ' ');
  if(spacePos == NULL){
    output_line(sp, "write: missing arguments");
    t_return(shellTaskHandle,-1);
    return;
  }
  *spacePos = 0;
  char * filename = argBuf;
  char * data = spacePos + 1;
  if(data[0]==0){
    output_line(sp, "write: missing data");
    t_return(shellTaskHandle,-1);
    return;
  }
  char path[128]; buildPath(path,sizeof(path), filename);
  fsStream(FS_OP_WRITE, path, data, 0, sp->outputQueue,sp);
  t_return(shellTaskHandle, 0);
}

void appendTask(void * params)
{
  ShellTaskParams * sp = (ShellTaskParams *)params;
  char * argBuf = sp->argBuf;
  char * spacePos = strchr(argBuf, ' ');
  if(spacePos == NULL){
    output_line(sp, "append: missing arguments");
    t_return(shellTaskHandle,-1);
    return;
  }
  *spacePos = 0;
  char * filename = argBuf;
  char * data = spacePos + 1;
  if(data[0]==0){
    output_line(sp, "append: missing data");
    t_return(shellTaskHandle,-1);
    return;
  }
  char path[128]; 
  buildPath(path,sizeof(path), filename);
  fsStream(FS_OP_APPEND, path, data, 0, sp->outputQueue,sp);
  t_return(shellTaskHandle, 0);
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
  // ...existing code (parsing)...
  if(args == NULL || strlen(args) == 0){
    strncpy(cmd, command, length);
  }
  if(shellParams){
    memset(shellParams->argBuf, 0, sizeof(shellParams->argBuf));
    strncpy(shellParams->argBuf, args, sizeof(shellParams->argBuf)-1);
  }
  // ...existing code...

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
  }

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

  fsStream(FS_OP_LIST, currentPath, NULL, 1, sp->fsOutputQueue, sp);
  // Wait for completion marker (optional)
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  for(;;){
      char ch;
      if(xQueueReceive(*sp->fsOutputQueue, &ch, pdMS_TO_TICKS(1000)) == pdTRUE)
      {
          Serial.printf("lsTask: Received char from queue: %d\n", ch);
          if(ch == '\x04') break; // EOT
          // forward to LCD or serial as your pipeline requires
          output_to_que(&ch, 1, sp);
          /*while(xQueueSend(*sp->outputQueue, &ch, pdMS_TO_TICKS(100)) != pdTRUE)
          {
            vTaskDelay(10 / portTICK_PERIOD_MS);
          }*/
          //vTaskDelay(10 / portTICK_PERIOD_MS);
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
  
  //*shellParams->fsOutputQueue = xQueueCreate(40, sizeof(char));
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
        
        //Serial.printf("Current Schemaphore (inputQ) holder: %p\n", xSemaphoreGetMutexHolder(inputQueueMutex));
        //Serial.printf("Current Schemaphore (OutputQ) holder: %p\n", xSemaphoreGetMutexHolder(outputQueueMutex));
        //Serial.printf("Current Schemaphore (OutputQ) holder: %p\n", xSemaphoreGetMutexHolder(fsMutex));



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
            //output_to_que(msg, strlen(msg), shellParams);
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