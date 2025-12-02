#include "filesys.h"
#include "shell.h"

//SemaphoreHandle_t fsMutex = NULL;
QueueHandle_t fsInQueue = NULL;

void fileSystemTask(void * params)
{

    // Littlefs defined in LittleFS.cpp
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        t_return(((FileSystemTaskParams *)params)->shellTaskHandle, -1);
    }
    //Serial.println("LittleFS Mounted Successfully");
    //fsMutex = xSemaphoreCreateMutex();
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
            switch(req.operation){
                case FS_OP_LIST:
                    listDir(LittleFS, req.path, req.levels, &req);
                    break;
                case FS_OP_READ:
                    readFile(LittleFS, req.path, &req);
                    break;
                case FS_OP_WRITE:
                    writeFile(LittleFS, req.path, req.data, &req);
                    break;
                case FS_OP_DELETE:
                    deleteFile(LittleFS, req.path, &req);
                    break;
                case FS_OP_MKDIR:
                    createDir(LittleFS, req.path, &req);
                    break;
                case FS_OP_RMDIR:
                    removeDir(LittleFS, req.path, &req);
                    break;
                case FS_OP_RENAME:
                    //req.levels is used as the destination path pointer here
                    renameFile(LittleFS, req.path, (const char *)req.data, &req);
                    break;
                default:
                    break;
            }
            //Notify the requesting task if needed
            char done = '\x04';
            xQueueSend(req.outputQueue, &done, 0);
            if(req.notifyTask != NULL){
                xTaskNotifyGive(req.notifyTask);
            }

        }
    }

    vTaskDelete(NULL);
}

/* TEST */

int listDir(fs::FS &fs, const char * dirname, uint8_t levels, FileSystemRequest * fsReq)
{
    File root = fs.open(dirname);
    if(!root)
    {
        //Serial.println("- failed to open directory");
        t_return(fsReq->notifyTask, -1);
    }
    if(!root.isDirectory())
    {
        //Serial.println(" - not a directory");
        t_return(fsReq->notifyTask, -1);
    }

    File file = root.openNextFile();
    while(file)
    {
        if(file.isDirectory())
        {
            //Serial.print("  DIR : ");
            Serial.printf("Listing directory: %s\n", file.path());
            const char * hdr = "DIR: ";
            for(size_t i=0;i<strlen(hdr);i++) 
                xQueueSend(fsReq->outputQueue,&hdr[i],0);
            for(size_t i=0;i<strlen(file.name());i++) 
                xQueueSend(fsReq->outputQueue,&file.name()[i],0);
            //xQueueSend(fsReq->outputQueue,"\n",0);
            //Serial.println(file.name());
            
            if(levels)
            {
                listDir(fs, file.path(), levels -1, fsReq);
            }
        } else 
        {
            //Serial.print("  FILE: ");
            const char * hdr = "FILE: ";
            for(size_t i=0;i<strlen(hdr);i++) 
                xQueueSend(fsReq->outputQueue,&hdr[i],0);
            for(size_t i=0;i<strlen(file.name());i++) 
                xQueueSend(fsReq->outputQueue,&file.name()[i],0);
            const char * sep = "SIZE: ";
            //xQueueSend(fsOutQueue,"\tSIZE: ",0);
            for (size_t i = 0; i < strlen(sep);i++)
                xQueueSend(fsReq->outputQueue,&sep[i],0);
            char sizeStr[16];
            snprintf(sizeStr, sizeof(sizeStr), "%u\n", file.size());
            for(size_t i=0;i<strlen(sizeStr);i++) 
                xQueueSend(fsReq->outputQueue,&sizeStr[i],0);
            //Serial.print("\tSIZE: ");
            //Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    //xQueueSend(fsReq->outputQueue, "\x04", 0);
    return 0;
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
    if(fs.rmdir(path))
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

int readFile(fs::FS &fs, const char * path, FileSystemRequest * fsReq)
{
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        const char *err = "ERR: open for read\n";
        for(size_t i=0;i<strlen(err);i++) xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    while(file.available()){
        char c = (char)file.read();
        xQueueSend(fsReq->outputQueue, &c, 0);
    }
    file.close();
    char nl='\n'; xQueueSend(fsReq->outputQueue,&nl,0);
    return 1;
}

int writeFile(fs::FS &fs, const char * path, const char * message, FileSystemRequest * fsReq)
{
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        const char *err = "ERR: open for write\n";
        for(size_t i=0;i<strlen(err);i++) xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    bool ok = file.print(message ? message : "");
    file.close();
    const char *msg = ok ? "OK: write\n" : "ERR: write\n";
    for(size_t i=0;i<strlen(msg);i++) xQueueSend(fsReq->outputQueue, &msg[i], 0);
    return ok ? 1 : -1;
}

int appendFile(fs::FS &fs, const char * path, const char * message, FileSystemRequest * fsReq)
{
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        const char *err = "ERR: open for append\n";
        for(size_t i=0;i<strlen(err);i++) xQueueSend(fsReq->outputQueue, &err[i], 0);
        return -1;
    }
    bool ok = file.print(message ? message : "");
    file.close();
    const char *msg = ok ? "OK: append\n" : "ERR: append\n";
    for(size_t i=0;i<strlen(msg);i++) xQueueSend(fsReq->outputQueue, &msg[i], 0);
    return ok ? 1 : -1;
}

int renameFile(fs::FS &fs, const char * path1, const char * path2, FileSystemRequest * fsReq)
{
    bool ok = fs.rename(path1, path2);
    const char *hdr = ok ? "OK: rename " : "ERR: rename ";
    for(size_t i=0;i<strlen(hdr);i++) xQueueSend(fsReq->outputQueue, &hdr[i], 0);
    for(size_t i=0;i<strlen(path1);i++) xQueueSend(fsReq->outputQueue, &path1[i], 0);
    const char *sep = " -> ";
    for(size_t i=0;i<strlen(sep);i++) xQueueSend(fsReq->outputQueue, &sep[i], 0);
    for(size_t i=0;i<strlen(path2);i++) xQueueSend(fsReq->outputQueue, &path2[i], 0);
    char nl='\n'; xQueueSend(fsReq->outputQueue,&nl,0);
    return ok ? 1 : -1;
}

int deleteFile(fs::FS &fs, const char * path, FileSystemRequest * fsReq)
{
    bool ok = fs.remove(path);
    const char *hdr = ok ? "OK: delete " : "ERR: delete ";
    for(size_t i=0;i<strlen(hdr);i++) xQueueSend(fsReq->outputQueue, &hdr[i], 0);
    for(size_t i=0;i<strlen(path);i++) xQueueSend(fsReq->outputQueue, &path[i], 0);
    char nl='\n'; xQueueSend(fsReq->outputQueue,&nl,0);
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