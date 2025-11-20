#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <SD.h>

#include "lcd.h"
#include "serial.h"
#include "shell.h"

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


typedef struct {
  TaskHandle_t shellTaskHandle;
  const char * basePath;

} FileSystemArgs;

void fileSystemTask(void * params){

}


void setup(){
  // put your setup code here, to run once: 
  Serial.begin(115200);

  static QueueHandle_t InputQueue = xQueueCreate(5, sizeof(char));
  static QueueHandle_t stdQueue = xQueueCreate(40, sizeof(char));


  inputQueueMutex = xSemaphoreCreateMutex();
  displayBufferMutex = xSemaphoreCreateMutex();

  //memset(displayBuffer, ' ', sizeof(displayBuffer));
  static ShellTaskParams shellParams = {
    .inputQueue = &InputQueue,
    .stdQueue = &stdQueue
  };

  static LCDTaskParams lcdParams = {
    .inputQueue = &stdQueue,
    .lcdCols = 16,
    .lcdRows = 2,
    .address = 0x27
  };
  static SerialInputTaskParams serialParams = {
    .shellTaskHandle = NULL,
    .inputQueue = &InputQueue
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
