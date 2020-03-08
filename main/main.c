/* Wear levelling and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This sample shows how to store files inside a FAT filesystem.
   FAT filesystem is stored in a partition inside SPI flash, using the 
   flash wear levelling library.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"


#include "file_core.h"


void app_main(void)
{

  xTaskCreate( file_thread, "file_thread", 2046, NULL, 5, NULL );
  
  while(fileCoreReady == 0)
  {
   portYIELD();
  }

   char n[30] = "hi sam";  
   commandQ_t qCommand;
   memset(&qCommand, 0, sizeof(qCommand));
    
   qCommand.command = FILE_DELETE_USER;
   qCommand.id      = 1;
   qCommand.name = n;
   file_thread_gate(&qCommand);

   qCommand.command = FILE_ADD_USER;
   sprintf(n, "eat my dic");
   qCommand.name = n;
   file_thread_gate(&qCommand);


   qCommand.command = FILE_PRINT_USERS;
   file_thread_gate(&qCommand);


  /*
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
    assert(partition != NULL);
    esp_partition_erase_range(partition, 0, 0x100000);

    ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }


    puts("Starting to Write now!");

    char fileName[30];
    employee_id_t temp;
    int i = 0;



    for(; i < MAX_EMPLOYEE; i++)
    {
        portYIELD();

       sprintf(fileName,"/spiflash/id_%d", i);
       FILE *z = fopen(fileName, "wb");

       if (z == NULL) 
        ESP_LOGE(TAG, "Failed to open file for writing");

       temp.id = i;
       sprintf(temp.name, "name + %d", i);
       int ret = fwrite(&temp, 1,64, z);
       
       if (ret == NULL){
        ESP_LOGE(TAG, "Failed to open file for writing");
      }
      fclose(z); 
    }
    ESP_LOGV(TAG ,"Starting to read now!");


    char line[128];
    for(i = 0; i < MAX_EMPLOYEE; i++)
    {
       sprintf(fileName,"/spiflash/id_%d", i);
   
      FILE *f = fopen(fileName, "rb");
      if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
      }
      fgets(line, sizeof(line), f);
      remove(fileName);
      fclose(f);
      
      memset((void*)&temp, 0, sizeof(temp));
      memcpy((void*)&temp, (const void*)line, sizeof(temp));


      printf("Employee ID: %d\n", temp.id);
      printf("Employee Name: %s\n", temp.name);
    }

  
    ESP_LOGI(TAG, "Opening file");
    FILE *f = fopen("/spiflash/hello.txt", "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "written using ESP-IDF %s\n", esp_get_idf_version());
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Open file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen("/spiflash/hello.txt", "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char line[128];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    // Unmount FATFS
    ESP_LOGI(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));

    ESP_LOGI(TAG, "Done");
*/
}


 
