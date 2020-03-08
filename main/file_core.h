#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/* define reqests to file core */
#define FILE_ADD_USER (0)
#define FILE_DEL_USER (1)
#define FILE_CMP_USER (2)
#define FILE_FORMAT_FLASH (3)
#define FILE_PRINT_USERS (4)
#define FILE_DELETE_USER (5)

/* define responses from file core */
#define FILE_RET_OK (0)
#define FILE_RET_FAIL (1)
#define FILE_RET_MEM_FULL (2)
#define FILE_RET_USER_NOT_EXIST (3)

/* employee tracking */
#define MAX_NAME_LEN_PLUS_NULL (49)
#define MAX_EMPLOYEE (512)

#define WEAR_PARTITION_SIZE (0x100000)

typedef struct{
  uint16_t id;
  char     name[MAX_NAME_LEN_PLUS_NULL];
}__attribute__ ((packed)) employee_id_t;


typedef struct{
  uint32_t command;
  char     *name;
  int      id;
}commandQ_t;


// Used for letting the rest of the system know file-core is 
// up
extern int fileCoreReady;

extern QueueHandle_t fileCommandQ, fileCommandQ_res;

void file_thread(const void * ptr);
int file_thread_gate(commandQ_t * cmd);

