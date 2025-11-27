#include "filesys.h"

void fileSystemTask(void * params)
{
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {

      Serial.println("Card Mount Failed");

      return;

    }

    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){

        Serial.println("No SD_MMC card attached");

        return;

    }


    Serial.print("SD_MMC Card Type: ");

    if(cardType == CARD_MMC){

        Serial.println("MMC");

    } else if(cardType == CARD_SD){

        Serial.println("SDSC");

    } else if(cardType == CARD_SDHC){

        Serial.println("SDHC");

    } else {

        Serial.println("UNKNOWN");

    }


    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);

    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

    vTaskDelete(NULL);



}