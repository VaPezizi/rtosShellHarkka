#include "filesys.h"
#include "shell.h"

SemaphoreHandle_t fsMutex = NULL;
QueueHandle_t fsInQueue = NULL;

void fileSystemTask(void * params)
{

    // Littlefs defined in LittleFS.cpp
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        //t_return(((FileSystemTaskParams *)params)->shellTaskHandle, -1);
        vTaskDelete(NULL);
    }
    //Serial.println("LittleFS Mounted Successfully");
    fsMutex = xSemaphoreCreateMutex();
    fsInQueue = xQueueCreate(10, sizeof(FileSystemRequest));
   // fsOutQueue = xQueueCreate(40, sizeof(char));

    //createDir(LittleFS, "/testi");
    //writeFile(LittleFS, "/testi/tiedosto.txt", "Hello from LittleFS!\n");
    //listDir(LittleFS, "/", 1);
    //deleteFile(LittleFS, "/testfile.bin");
    // readFile(LittleFS, "/testi/tiedosto.txt");
    // deleteFile(LittleFS, "/testi/tiedosto.txt");
    // removeDir(LittleFS, "/testi");

    //testFileIO(LittleFS, "/testfile.bin");
    while(1){
        FileSystemRequest req;
        if(xQueueReceive(fsInQueue, &req, portMAX_DELAY) == pdTRUE){
            //Got a request
            int val;
            if(xSemaphoreTake(fsMutex, pdMS_TO_TICKS(500)) != pdTRUE){
                Serial.println("FileSystemTask: Failed to take fsMutex semaphore!");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                continue;
            }
            switch (req.operation)
            {
            case FS_OP_LIST:
                val = listDir(LittleFS, req.path, req.levels, &req);
                break;
            case FS_OP_READ:
                val = readFile(LittleFS, req.path, &req);
                break;
            case FS_OP_WRITE:
                val = writeFile(LittleFS, req.path, req.data, &req);
                break;
            case FS_OP_DELETE:
                val = deleteFile(LittleFS, req.path, &req);
                break;
            case FS_OP_MKDIR:
                val = createDir(LittleFS, req.path, &req);
                break;
            case FS_OP_RMDIR:
                val = removeDir(LittleFS, req.path, &req);
                break;
            case FS_OP_RENAME:
                // req.levels is used as the destination path pointer here
                val = renameFile(LittleFS, req.path, (const char *)req.data, &req);
                break;
            case FS_OP_APPEND:
                val = appendFile(LittleFS, req.path, req.data, &req);
                break;
            default:
                break;
            }
            //Notify the requesting task if needed
            char done = '\x04';
            xQueueSend(req.outputQueue, &done, 0);
            if(req.notifyTask != NULL){
                Serial.printf("FileSystemTask: Notifying task %p of completion with value %d\n", req.notifyTask, val);
                xTaskNotify(req.notifyTask, val, eSetValueWithOverwrite);
            }
            xSemaphoreGive(fsMutex);
        }
    }

    vTaskDelete(NULL);
}

/* TEST */

void safe_output(QueueHandle_t * outputQueue, const char * str)
{
    if(!str) return;

    for(size_t i=0;i<strlen(str);i++) 
    {
        if(uxQueueSpacesAvailable(*outputQueue) == 0)
        {
            // wait for space
            vTaskDelay(50 / portTICK_PERIOD_MS);
            i--; // retry
        }else
        {
            xQueueSend(*outputQueue,&str[i],50);
        }
    }
}

int listDir(fs::FS &fs, const char * dirname, uint8_t levels, FileSystemRequest * fsReq)
{
    File root = fs.open(dirname);
    if(!root)
    {
        Serial.printf("ERR: failed to open directory %s\n", dirname);
        safe_output(&fsReq->outputQueue, "ERR: open dir\n");
        return -1;
    }
    if(!root.isDirectory())
    {
        Serial.printf("ERR: %s is not a directory\n", dirname);
        safe_output(&fsReq->outputQueue, "ERR: not a dir\n");
        root.close();
        return -1;
    }

    File file = root.openNextFile();
    while(file)
    {
        char pathBuf[128];
        char nameBuf[64];
        bool isDir = file.isDirectory();
        uint32_t size = file.size();
        
        // Copy before file object changes
        strncpy(pathBuf, file.path(), sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        
        strncpy(nameBuf, file.name(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        
        file.close(); // Close BEFORE recursing
        
        if(isDir)
        {
            safe_output(&fsReq->outputQueue, "DIR: ");
            safe_output(&fsReq->outputQueue, nameBuf);
            safe_output(&fsReq->outputQueue, "\n");
            Serial.printf("DIR: %s\n", nameBuf);
            
            if(levels > 0)
            {
                listDir(fs, pathBuf, levels - 1, fsReq);
            }
        }
        else
        {
            safe_output(&fsReq->outputQueue, "FILE: ");
            safe_output(&fsReq->outputQueue, nameBuf);
            safe_output(&fsReq->outputQueue, "  SIZE: ");
            
            char sizeStr[32];
            snprintf(sizeStr, sizeof(sizeStr), "%u\n", size);
            safe_output(&fsReq->outputQueue, sizeStr);
            
            Serial.printf("FILE: %s SIZE: %u\n", nameBuf, size);
        }
        
        file = root.openNextFile();
    }
    
    root.close();
    return 1;
}

int createDir(fs::FS &fs, const char * path, FileSystemRequest * fsReq)
{
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path))
    {
        return 1;
        // Serial.println("Dir created");
    }else 
    {
        return -1;
        //Serial.println("mkdir failed");
    }
}

int removeDir(fs::FS &fs, const char * path, FileSystemRequest * fsReq)
{
    Serial.printf("Removing Dir: %s\n", path);
    
    // Check if directory is empty first
    File dir = fs.open(path);
    if(!dir || !dir.isDirectory()){
        const char *err = "ERR: not a directory\n";
        for(size_t i=0;i<strlen(err);i++) xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    
    File file = dir.openNextFile();
    if(file){
        const char *err = "ERR: directory not empty\n";
        for(size_t i=0;i<strlen(err);i++) xQueueSend(fsReq->outputQueue, &err[i], 0);
        file.close();
        dir.close();
        return -1;
    }
    dir.close();
    
    if(fs.rmdir(path))
    {
        const char *msg = "Directory removed\n";
        for(size_t i=0;i<strlen(msg);i++) 
            xQueueSend(fsReq->outputQueue, &msg[i], 0);
        return 1;
    }
    else
    {
        const char *err = "ERR: rmdir failed\n";
        for(size_t i=0;i<strlen(err);i++) 
            xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
}

int readFile(fs::FS &fs, const char * path, FileSystemRequest * fsReq)
{
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        const char *err = "ERR: open for read\n";
        for(size_t i=0;i<strlen(err);i++) 
            xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    while(file.available()){
        char c = (char)file.read();
        xQueueSend(fsReq->outputQueue, &c, 0);
    }
    file.close();
    char nl='\n'; 
    xQueueSend(fsReq->outputQueue,&nl,0);
    return 1;
}

int writeFile(fs::FS &fs, const char * path, const char * message, FileSystemRequest * fsReq)
{
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        const char *err = "ERR: open for write\n";
        for(size_t i=0;i<strlen(err);i++) 
            xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    bool ok = file.print(message ? message : "");
    file.close();

    if(message && message[0] == '\0')
        ok = true; // touching a file is always successful

    const char *msg = ok ? "OK: write\n" : "ERR: write\n";
    for(size_t i=0;i<strlen(msg);i++) 
        xQueueSend(fsReq->outputQueue, &msg[i], 0);
    return ok ? 1 : -1;
}

int appendFile(fs::FS &fs, const char * path, const char * message, FileSystemRequest * fsReq)
{
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        const char *err = "ERR: open for append\n";
        for(size_t i=0;i<strlen(err);i++) 
            xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    bool ok = file.print(message ? message : "");
    file.close();
    const char *msg = ok ? "OK: append\n" : "ERR: append\n";
    for(size_t i=0;i<strlen(msg);i++) 
        xQueueSend(fsReq->outputQueue, &msg[i], 0);
    return ok ? 1 : -1;
}

int renameFile(fs::FS &fs, const char * path1, const char * path2, FileSystemRequest * fsReq)
{
    bool ok = fs.rename(path1, path2);
    const char *hdr = ok ? "OK: rename " : "ERR: rename ";
    for(size_t i=0;i<strlen(hdr);i++) 
        xQueueSend(fsReq->outputQueue, &hdr[i], 0);
    for(size_t i=0;i<strlen(path1);i++) 
        xQueueSend(fsReq->outputQueue, &path1[i], 0);
    const char *sep = " -> ";
    for(size_t i=0;i<strlen(sep);i++) 
        xQueueSend(fsReq->outputQueue, &sep[i], 0);
    for(size_t i=0;i<strlen(path2);i++) 
        xQueueSend(fsReq->outputQueue, &path2[i], 0);
    char nl='\n'; 
        xQueueSend(fsReq->outputQueue,&nl,0);
    return ok ? 1 : -1;
}

int deleteFile(fs::FS &fs, const char * path, FileSystemRequest * fsReq)
{
    bool ok = fs.remove(path);
    const char *hdr = ok ? "OK: delete " : "ERR: delete ";
    for(size_t i=0;i<strlen(hdr);i++) 
        xQueueSend(fsReq->outputQueue, &hdr[i], 0);
    for(size_t i=0;i<strlen(path);i++) 
        xQueueSend(fsReq->outputQueue, &path[i], 0);
    char nl='\n'; 
    xQueueSend(fsReq->outputQueue,&nl,0);
    return ok ? 1 : -1;
}

void testFileIO(fs::FS &fs, const char * path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++)
    {
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len)
        {
            size_t toRead = len;
            if(toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else 
    {
        Serial.println("- failed to open file for reading");
    }
}