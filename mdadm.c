#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

int single_mount_call = 0; // 1 is mounted, 0 is unmounted

int mdadm_mount(void) {
  uint32_t tmp = JBOD_MOUNT << 12; // takes the 12th digit onwards (the command)
  if (single_mount_call == 1) {
    return -1;
  }
  if (jbod_operation(tmp, NULL) == 0) { // success
    single_mount_call = 1;
    return 1; 
  } 
  else {
    return -1;
  }
}

int mdadm_unmount(void) {
  uint32_t tmp = 0;
  tmp = JBOD_UNMOUNT << 12;
  if (single_mount_call == 0) { // unmounted
    return -1;
  }
  if (jbod_operation(tmp, NULL) == 0) {
    single_mount_call = 0;
    return 1;
  } 
  else {
    return -1;
  }
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  
  if (single_mount_call == 0) { 
    return -3;
  }
  else if (read_len > 1024) {
    return -2;
  }
  else if ((start_addr + read_len) > 1048576) {
    return -1;
  }
  else if ((read_buf == NULL) && (read_len != 0)) {
    return -4;
  }
  
  int current_location = start_addr;
  int bytes_read = 0;

  while (bytes_read < read_len) {
        uint32_t disk_num = current_location / JBOD_DISK_SIZE; // which disk it is
        uint32_t block_num = (current_location % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE; // which block it is
        uint32_t offset = (current_location % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE; // the leftovers bytes

        uint32_t seek_disk = disk_num | block_num << 4 | (JBOD_SEEK_TO_DISK << 12); // Seeking to the correct disk
        if (jbod_operation(seek_disk, NULL) == -1) {
          return -4;
        }
        
        uint32_t seek_block = disk_num | block_num << 4 | (JBOD_SEEK_TO_BLOCK << 12) ; // Seeking to the correct block
        if (jbod_operation(seek_block, NULL) == -1) {
          return -4;
        }
      
        uint8_t block[JBOD_BLOCK_SIZE]; 
        uint32_t reading = disk_num | block_num << 4 | JBOD_READ_BLOCK << 12; // Reading the block 
        if (jbod_operation(reading, block) == -1) {
          return -4;
        }
        
        // to determine if the amount of bytes left to read exceeds the current bytes left in the block
        uint32_t bytes_left = JBOD_BLOCK_SIZE - offset;
        if (bytes_left > read_len - bytes_read) { 
            bytes_left = read_len - bytes_read;
        }

        memcpy(read_buf + bytes_read, block + offset, bytes_left); // dest, source, length

        bytes_read += bytes_left;
        current_location += bytes_left;
        
    } 
  
  return read_len;
  
} 


