#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>

#include <LiquidCrystal_I2C.h>
#include "lcd.h"

void lcdTask(void * params)
{
  
  LCDTaskParams * lcdparams = (LCDTaskParams *) params;

  static LiquidCrystal_I2C lcd(lcdparams->address, lcdparams->lcdCols, lcdparams->lcdRows);
  //static LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD I2C address
  uint8_t currentBufIndex, currentRow = 0;
  char localBuffer[32] = {0};

  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.clear();
  lcd.cursor();

  while (1)
  {
    /*if(1){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Hello, AAAALLLLL World!");
      Serial.println("LCD Task: Displaying welcome message");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      
      continue;
    }*/

    //uint32_t notificationValue;
    if(xSemaphoreTake(outputQueueMutex, pdTICKS_TO_MS(100)) == pdTRUE)
    {
      char ch;
      if(xQueueReceive(*lcdparams->inputQueue, &ch, (TickType_t)10) == pdTRUE)
      {
        if(ch == '\b')
        { //Handle backspace
          if(currentBufIndex > 0)
          {
            currentBufIndex--;
            lcd.setCursor(currentBufIndex, currentRow);
            lcd.print(' '); //Overwrite with space
            lcd.setCursor(currentBufIndex, currentRow);
          }
          xSemaphoreGive(outputQueueMutex);
          vTaskDelay(50 / portTICK_PERIOD_MS);
          continue; //Skip further processing for backspace
        }
        if(ch == '\x1b' /*|| ch== '\n' || ch == '\r'*/)
        { //Clear screen command
          lcd.clear();
          lcd.setCursor(0,0);
          currentBufIndex = 0;
          currentRow = 0;
          xSemaphoreGive(outputQueueMutex);
          vTaskDelay(50 / portTICK_PERIOD_MS);
          continue;
        }
        if(ch == '\n' || ch == '\r')
        { //New line
          currentBufIndex = 0;
          
          currentRow = (currentRow + 1) % 2;
          lcd.setCursor(0, currentRow);
          lcd.print("                ");
          lcd.setCursor(0, currentRow);

          xSemaphoreGive(outputQueueMutex);
          vTaskDelay(50 / portTICK_PERIOD_MS);
          continue;
        }
        if(ch == '\x04')
        { //End of transmission
          xSemaphoreGive(outputQueueMutex);
          vTaskDelay(50 / portTICK_PERIOD_MS);
          continue;
        }
        currentBufIndex++;
        if(currentBufIndex > 16)
        {
          currentBufIndex = 0;
          if(currentRow == 1)
          {
            lcd.clear();
          }
          currentRow = (currentRow + 1) % 2;
          lcd.setCursor(0, currentRow);
          lcd.print("                ");
          lcd.setCursor(0, currentRow);
          //lcd.clear();
        }
        lcd.print(ch);
        Serial.printf("LCD Task received char: %d\n", ch);

        //if(currentBufIndex == 16){
        //  currentBufIndex = 0;
        //  currentRow = (currentRow + 1) % 2;
        //  lcd.setCursor(0, currentRow);
          //lcd.clear();
        //}
      }
      xSemaphoreGive(outputQueueMutex);
    }else
    {
      Serial.println("LCD Task: Failed to take outputQueueMutex");
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  
}