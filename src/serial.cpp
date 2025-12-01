#include "serial.h"

void serialInputTask(void * params){
  SerialInputTaskParams * serialParams = (SerialInputTaskParams *) params;
  
  while (1) {
        if (Serial.available() > 0) {
            char ch = Serial.read(); // Read one character from the serial buffer
            Serial.printf("Serial Task: Read char: %c\n", ch);

            //This seemed smarter at first, but quickly became a problem

            // Notify the shellTask and pass the character as the notification value
            /*if (xTaskNotify(serialParams->shellTaskHandle, (uint32_t)ch, eSetValueWithOverwrite) == pdPASS) {
                Serial.printf("Serial Task: Notified shellTask with char: %c\n", ch);
            } else {
                Serial.println("Serial Task: Failed to notify shellTask");
            }*/
            
            
            if(xSemaphoreTake(inputQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE){
                if(xQueueSend(*serialParams->inputQueue, &ch, pdMS_TO_TICKS(100)) != pdTRUE){
                    Serial.println("Serial Task: Failed to send char to inputQueue");
                }
                xSemaphoreGive(inputQueueMutex);
            }else{
                Serial.println("Serial Task: Failed to take inputQueueMutex");
                Serial.printf("Current owner of inputQueueMutex: %p\n", xSemaphoreGetMutexHolder(inputQueueMutex));
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Small delay to avoid hogging the CPU
    }
}
