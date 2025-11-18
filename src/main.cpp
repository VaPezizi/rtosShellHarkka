#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <SD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD I2C address

static SemaphoreHandle_t inputQueueMutex;
static SemaphoreHandle_t displayBufferMutex;

//static char displayBuffer[32] = {0};


typedef struct {
  QueueHandle_t * inputQueue;
  uint8_t lcdAddress;
  uint8_t lcdCols;
  uint8_t lcdRows;
} LCDTaskParams;

typedef struct {
  QueueHandle_t * inputQueue;
  QueueHandle_t * stdQueue;
  TaskHandle_t lcdTaskHandle;
  TaskHandle_t fileSystemTaskHandle;
} ShellTaskParams;

typedef struct {
  TaskHandle_t shellTaskHandle;
} SerialInputTaskParams;

void fileSystemTask(void * params){

}

void serialInputTask(void * params){
  SerialInputTaskParams * serialParams = (SerialInputTaskParams *) params;

  while (1) {
        if (Serial.available() > 0) {
            char ch = Serial.read(); // Read one character from the serial buffer
            Serial.printf("Serial Task: Read char: %c\n", ch);

            // Notify the shellTask and pass the character as the notification value
            if (xTaskNotify(serialParams->shellTaskHandle, (uint32_t)ch, eSetValueWithOverwrite) == pdPASS) {
                Serial.printf("Serial Task: Notified shellTask with char: %c\n", ch);
            } else {
                Serial.println("Serial Task: Failed to notify shellTask");
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS); // Small delay to avoid hogging the CPU
    }
}

void shellTask(void * params){
  TaskHandle_t currentTask;
  ShellTaskParams * shellParams = (ShellTaskParams *) params;

  char inputBuffer[64] = {0};
  uint8_t bufferIndex = 0;
  char lastChar = 0;
  
  while(1){

    //Wait for notification from serial input task
    uint32_t notifiedValue;
    if(xTaskNotifyWait(0, 0, &notifiedValue, pdMS_TO_TICKS(100)) == pdTRUE){
        char ch = (char)notifiedValue;
        Serial.printf("Shell Task: Received char: %c\n", ch);

        
        if(ch == '\n' || ch == '\r'){
            inputBuffer[bufferIndex] = '\0'; // Null-terminate the string
            Serial.printf("Shell Task: Complete input received: %s\n", inputBuffer);
            bufferIndex = 0; // Reset buffer index for next input
            memset(inputBuffer, 0, sizeof(inputBuffer)); // Clear the input buffer
            lastChar = 0;
            ch = '\x1b';  // Sending clear screen command to LCD task
        }

        //Echo back to stdQueue
        if(xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
            if(xQueueSend(*shellParams->stdQueue, &ch, pdMS_TO_TICKS(100)) != pdTRUE){
                Serial.println("Shell Task: Failed to send char to stdQueue");
            }
            xSemaphoreGive(inputQueueMutex);
        }
        

        //Build input buffer
        
        else{
            if(bufferIndex < sizeof(inputBuffer) - 1){
                inputBuffer[bufferIndex++] = ch;
            }
        }
    }

    //If just in shell
    if(currentTask == NULL){
        
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void lcdTask(void * params){
  
  LCDTaskParams * lcdparams = (LCDTaskParams *) params;
  uint8_t currentBufIndex, currentRow = 0;
  char localBuffer[32] = {0};

  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.clear();
  lcd.cursor();

  while (1)
  {
    uint32_t notificationValue;
    if(xSemaphoreTake(displayBufferMutex, pdTICKS_TO_MS(100)) == pdTRUE){
      char ch;
      if(xQueueReceive(*lcdparams->inputQueue, &ch, (TickType_t)10) == pdTRUE){
        if(ch == '\x1b'){ //Clear screen command
          lcd.clear();
          lcd.setCursor(0,0);
          currentBufIndex = 0;
          currentRow = 0;
          xSemaphoreGive(displayBufferMutex);
          continue;
        }
        currentBufIndex++;
        if(currentBufIndex > 16){
          currentBufIndex = 0;
          currentRow = (currentRow + 1) % 2;
          lcd.setCursor(0, currentRow);
          //lcd.clear();
        }
        lcd.print(ch);
        Serial.printf("LCD Task received char: %c\n", ch);

        //if(currentBufIndex == 16){
        //  currentBufIndex = 0;
        //  currentRow = (currentRow + 1) % 2;
        //  lcd.setCursor(0, currentRow);
          //lcd.clear();
        //}
      }
      xSemaphoreGive(displayBufferMutex);
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  
}

void setup(){
  // put your setup code here, to run once: 
  Serial.begin(115200);

  static QueueHandle_t InputQueue = xQueueCreate(5, sizeof(char));
  static QueueHandle_t stdQueue = xQueueCreate(5, sizeof(char));


  inputQueueMutex = xSemaphoreCreateMutex();
  displayBufferMutex = xSemaphoreCreateMutex();

  //memset(displayBuffer, ' ', sizeof(displayBuffer));
  static ShellTaskParams shellParams = {
    .inputQueue = &InputQueue,
    .stdQueue = &stdQueue
  };

  static LCDTaskParams lcdParams = {
    .inputQueue = &stdQueue
  };
  static SerialInputTaskParams serialParams = {
    .shellTaskHandle = NULL
};

  if(InputQueue == NULL || inputQueueMutex == NULL){
    Serial.println("Could not create the queue");
    
  }
  else{
    xTaskCreate(shellTask, "Shell Task", 4096, &shellParams, 1, &serialParams.shellTaskHandle);
    xTaskCreate(lcdTask, "LCD Task", 2048, &lcdParams, 1, NULL);
    xTaskCreate(serialInputTask, "Serial Task", 2048, &serialParams, 1, NULL);
    
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  
  
}
