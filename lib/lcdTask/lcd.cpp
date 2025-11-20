#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>

#include <LiquidCrystal_I2C.h>
#include "lcd.h"

void lcdTask(void * params){
  
  LCDTaskParams * lcdparams = (LCDTaskParams *) params;

  static LiquidCrystal_I2C lcd(lcdparams->lcdAddress, lcdparams->lcdCols, lcdparams->lcdRows);
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
        if(ch == '\b'){ //Handle backspace
          if(currentBufIndex > 0){
            currentBufIndex--;
            lcd.setCursor(currentBufIndex, currentRow);
            lcd.print(' '); //Overwrite with space
            lcd.setCursor(currentBufIndex, currentRow);
          }
          xSemaphoreGive(displayBufferMutex);
          continue; //Skip further processing for backspace
        }
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