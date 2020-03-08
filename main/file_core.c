#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>


#include "file_core.h"

// Base path
static const char *base_path = "/spiflash";

// Debug tag
static const char TAG[30] = "FILE_CORE";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

QueueHandle_t fileCommandQ, fileCommandQ_res;
BaseType_t xStatus;
int fileCoreReady;
static SemaphoreHandle_t fileCommandMutex;

int file_thread_gate(commandQ_t * cmd)
{
  int ret;
  xSemaphoreTake(fileCommandMutex, portMAX_DELAY );

  // Send the command
  xQueueSend(fileCommandQ, cmd, 0);

  // Wait for the response
  xQueueReceive(fileCommandQ_res, &ret, portMAX_DELAY);

  xSemaphoreGive(fileCommandMutex);

  return ret;
}


// Function will loop through all posible users IDs
// And stop at the first one that does not exist
static int add_user(commandQ_t * cmd)
{ 
  ESP_LOGI(TAG, "Adding user");

  int i, ret;
  char fileName[30];
  employee_id_t temp;
  
  if (strlen(cmd->name) > MAX_NAME_LEN_PLUS_NULL - 1) //strlen does not count NULL char
  {
    ESP_LOGE(TAG, "Name was too long");
    return FILE_RET_FAIL;
  }

  for(i = 0; i < MAX_EMPLOYEE; i++)
  {
    sprintf(fileName,"/spiflash/id_%d", i);
    ret = access(fileName, F_OK);

    if(ret == NULL)
    {
      // This user already exists 
      continue;
    }
    if(errno == ENOENT)
    {
      //user does not eixst
      break;
    }
  
    ESP_LOGE(TAG, "Unexpected errno - %d", errno);
    return FILE_RET_FAIL;
  }

  // Out of memory
  if (i == MAX_EMPLOYEE)
  {
    return FILE_RET_MEM_FULL;
  }

  // i will be initilized from the above loop
  sprintf(fileName,"/spiflash/id_%d", i);
  FILE *z = fopen(fileName, "wb");

  if (z == NULL) 
  {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return FILE_RET_FAIL;
  }

  // Fomat the user ID + Name to write to flash
  temp.id = i;
  sprintf(temp.name, cmd->name);

  ret = fwrite(&temp, 1, sizeof(employee_id_t), z);
  fclose(z); 

  if (ret == NULL)
  {
    ESP_LOGE(TAG, "Failed write to file");
    return FILE_RET_FAIL;
  }

  if (ret != sizeof(employee_id_t))
  {
    ESP_LOGE(TAG, "Only partially wrote to file, %d bytes", ret);
    return FILE_RET_FAIL;
  }
  
  return FILE_RET_OK;
}

static int delete_user(commandQ_t * cmd)
{ 
  ESP_LOGI(TAG, "Deleting user %d ", cmd->id);
  int ret;
  char fileName[30];
  employee_id_t temp;
  
  if ((cmd->id < 0) || (cmd->id > MAX_EMPLOYEE))
  {
    ESP_LOGE(TAG, "ID out of range: ID == %d", cmd->id);
    return FILE_RET_FAIL;
  }
  
  // If a user exists, a file will exist in the form of
  // "id_x", where X is the users ID
  // IE: id_34 for person 34 etc
  sprintf(fileName,"/spiflash/id_%d", cmd->id);
  ret = access(fileName, F_OK); //checks to see if file exists

  if(ret == NULL)
  {  
    remove(fileName);
    return FILE_RET_OK;
  }

  if(errno == ENOENT)
  {
    ESP_LOGE(TAG, "User %d does not exist, can't delete", cmd->id);
    return FILE_RET_USER_NOT_EXIST;
  }

  ESP_LOGE(TAG, "Unexpected errno - %d", errno);
  return FILE_RET_FAIL;
}

static int print_users()
{  
  ESP_LOGI(TAG, "Printing users");

  char employee[64];
  char fileName[30];
  int ret;
  int i;

  for(i = 0; i < MAX_EMPLOYEE; i++)
  {
    sprintf(fileName,"/spiflash/id_%d", i);
    ret =access(fileName, F_OK);
    if(ret == NULL)
    {  
      FILE *f = fopen(fileName, "rb");
      if (f == NULL)
      {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return FILE_RET_FAIL;
      } 
      fgets(employee, sizeof(employee_id_t), f);
      employee_id_t e;

      memset((void*)&e, 0, sizeof(employee_id_t));
      memcpy((void*)&e, (const void*)employee, sizeof(employee_id_t));

      printf("\nEmployee ID: %d\n", e.id);
      printf("Employee Name: %s\n\n", e.name);
     
      continue; 
    }
    if(errno == ENOENT)
    {
      continue;
    }
    // should not get here
    ESP_LOGE(TAG, "Strang errno: %d:  giving up", errno);
    return FILE_RET_FAIL;
  }
  return FILE_RET_OK;
}

static esp_err_t format_flash()
{
  ESP_LOGI(TAG, "Formatting file system");
  const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
  if (partition == NULL)
  {
    return FILE_RET_FAIL;
  }

  if (ESP_OK != esp_partition_erase_range(partition, 0, WEAR_PARTITION_SIZE))
  {
    return FILE_RET_FAIL;
  }
  return FILE_RET_OK;
}

void file_thread(const void * ptr)
{
  commandQ_t commandQ_cmd; 

  ESP_LOGI(TAG, "Mounting FAT filesystem");
  // To mount device we need name of device partition, define base_path
  // and allow format partition in case if it is new one and was not formated before
  const esp_vfs_fat_mount_config_t mount_config = {
          .max_files = 4,
          .format_if_mount_failed = true,
          .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
  };

  esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
  if (err != ESP_OK) 
  {
      ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
      assert(0);
      //TODO: Gracefully handle this....
  }

  fileCommandQ     = xQueueCreate(1 , sizeof(commandQ_t));
  fileCommandQ_res = xQueueCreate(1 , sizeof(int32_t));

  fileCommandMutex =  xSemaphoreCreateMutex();

  //let the rest of the system know we file-core is ready
  fileCoreReady = 1;


  for(;;)
  {
    
    int ret;
    // Wait for command
    xStatus = xQueueReceive(fileCommandQ, &commandQ_cmd,portMAX_DELAY);
    if(xStatus == NULL){
      ESP_LOGE(TAG, "Failed to create Queue..");
    }

    switch (commandQ_cmd.command)
    {
      case FILE_ADD_USER:
        ret = add_user(&commandQ_cmd);
        xQueueSend(fileCommandQ_res, &ret, 0);
        break; 
      case FILE_FORMAT_FLASH:
        ret = format_flash();
        xQueueSend(fileCommandQ_res, &ret, 0);
        break;
      case FILE_PRINT_USERS:
        ret = print_users();
        xQueueSend(fileCommandQ_res, &ret, 0);
        break;
      case FILE_DELETE_USER:
        ret = delete_user(&commandQ_cmd);
        xQueueSend(fileCommandQ_res, &ret, 0);
        break;
    }
  }
}
