#ifndef SDPFSSTRUCTS_H
#define SDPFSSTRUCTS_H

#include "stdint.h"



struct directoryEntry {
  uint8_t  name[11];
          /** Entry attributes.
           *
           * The upper two bits of the attribute byte are reserved and should
           * always be set to 0 when a file is created and never modified or
           * looked at after that.  See defines that begin with DIR_ATT_.
           */
  uint8_t  attributes;
  uint32_t fileSize;
  uint32_t firstCluster;

  uint8_t  padding[12];
}__attribute__((packed));

typedef struct directoryEntry dir_t;
//------------------------------------------------------------------------------

struct PFS_boot{
  uint8_t  sectorsPerCluster;
  uint32_t totalSectors;
  uint32_t FTableSectors;
  uint32_t rootStartCluster;
  uint16_t bytesPerSector;
  uint16_t rootDirEntryCount;
  char     volumeLabel[11];

  uint8_t  padding[512-28];
}__attribute__((packed));

typedef struct PFS_boot pfs_boot_t;

//------------------------------------------------------------------------------


struct directoryEntry_info {


  dir_t *dirEntry;
  int clusterPosition;
  int offset;

}__attribute__((packed));

typedef struct directoryEntry_info dir_info_t;
//------------------------------------------------------------------------------



#endif // SDPFSSTRUCTS_H
