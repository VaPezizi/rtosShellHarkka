#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "lcd.h"
#include "serial.h"
#include "shell.h"
#include "filesys.h"

volatile SemaphoreHandle_t inputQueueMutex = NULL;
volatile SemaphoreHandle_t outputQueueMutex = NULL;

//LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD I2C address


//static char displayBuffer[32] = {0};

/*
typedef struct {
  QueueHandle_t * inputQueue;
  uint8_t lcdAddress;
  uint8_t lcdCols;
  uint8_t lcdRows;
} LCDTaskParams;
*/


void setup(){
  // put your setup code here, to run once: 
  Serial.begin(115200);

  static QueueHandle_t InputQueue = xQueueCreate(10, sizeof(char));
  static QueueHandle_t outQueue = xQueueCreate(40, sizeof(char));
  static QueueHandle_t fsOutQueue = xQueueCreate(50, sizeof(char));

  inputQueueMutex = xSemaphoreCreateMutex();
  outputQueueMutex = xSemaphoreCreateMutex();

  TaskHandle_t lcdHandle = NULL;
  TaskHandle_t serialHandle = NULL;

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  static FileSystemTaskParams fsParams = {
    .shellTaskHandle = NULL,
  };

  //memset(displayBuffer, ' ', sizeof(displayBuffer));
  static ShellTaskParams shellParams = {
    .inputQueue = &InputQueue,
    .outputQueue = &outQueue,
    .fsOutputQueue = &fsOutQueue,
  };

  static LCDTaskParams lcdParams = {
    .inputQueue = &outQueue,
    .lcdCols = 16,
    .lcdRows = 2,
    .address = 0x27
  };
  static SerialInputTaskParams serialParams = {
    .shellTaskHandle = NULL,
    .inputQueue = &InputQueue
  };

  if(InputQueue == NULL || inputQueueMutex == NULL || outQueue == NULL || outputQueueMutex == NULL){
    Serial.println("Could not create queues or mutexes!");
    
    while(1) 
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  

  else{
    Serial.println("Tasks starting...");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(shellTask, "Shell Task", 8192, &shellParams, 1, &serialParams.shellTaskHandle);
    Serial.printf("Shell Task Handle: %p\n", serialParams.shellTaskHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    xTaskCreate(lcdTask, "LCD Task", 2048, &lcdParams, 1, &lcdHandle);
    Serial.printf("LCD Task Handle: %p\n", lcdHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    xTaskCreate(serialInputTask, "Serial Task", 2048, &serialParams, 1, &serialHandle);
    Serial.printf("Serial Task Handle: %p\n", serialHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    xTaskCreate(fileSystemTask, "File System Task", 4096, &fsParams, 1, &fsParams.shellTaskHandle);
    Serial.printf("File System Task Handle: %p\n", fsParams.shellTaskHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  
  
}
