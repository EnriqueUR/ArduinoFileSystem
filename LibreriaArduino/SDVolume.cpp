#include <SDVolume.h>

bool SDVolume::init(SDCard* dev) {
  uint32_t totalSectors;
  uint32_t volumeStartBlock = 0;
  sb_t* sb;
  sdCard_ = dev;

  sdCard_->readBlock(volumeStartBlock, cacheBuffer_.data);
  blockNumber_ = volumeStartBlock;

  sb = &(cacheBuffer_.superblock);
  if (sb->bytesPerSector != 512 || sb->sectorsPerCluster == 0) {
    ////serial.println("Error en SDVolume.cpp, bytesPerSector != 512");
    goto fail;
  }
  sectorsPerCluster_ = sb->sectorsPerCluster;

  clusterSizeShift_ = 0;
  while (sectorsPerCluster_ != (1 << clusterSizeShift_)) {
    if (clusterSizeShift_++ > 7) {
      ////serial.println("Error en SDVolume.cpp, sectorsPerCluster != 2^x");
      goto fail;
    }
  }
  sectorsPerFTable_ = sb->fTableSectors << clusterSizeShift_;

  fTableStartBlock_ = 1*sectorsPerCluster_;

  rootDirEntryCount_ = sb->rootDirEntryCount;

  rootDirStart_ = sb->rootStartCluster;

  dataStartSector_ = rootDirStart_ + 1;

  totalSectors = sb->totalSectors;

  clusterCount_ = (totalSectors - (dataStartSector_ - volumeStartBlock)) >> clusterSizeShift_;
  return true;

 fail:
  return false;
}

bool SDVolume::freeChain(uint32_t cluster) {
  uint32_t next;

  do {
    if (!getNextTableEntry(cluster, &next)) {
      ////serial.println("Error en SDVolume.cpp, getNextTableEntry fallo");
      goto fail;
    }
    if (!setTableEntry(cluster, 0)) {
      goto fail;
    }

    cluster = next;
  } while (!isEOC(cluster));

  return true;

 fail:
  return false;
}

cache_t* SDVolume::cacheFetch(uint32_t blockNumber, uint8_t options) {
  if (blockNumber_ != blockNumber) {
    // if (!cacheSync()) {
    //   DBG_FAIL_MACRO;
    //   goto fail;
    // }
    if (!(options & CACHE_OPTION_NO_READ)) {
      if (!sdCard_->readBlock(blockNumber, cacheBuffer_.data)) {
        goto fail;
      }
    }
    //cacheStatus_ = 0;
    blockNumber_ = blockNumber;
  }
  //cacheStatus_ |= options & CACHE_STATUS_MASK;
  return &cacheBuffer_;

 fail:
  return 0;
}

cache_t* SDVolume::cacheFetchFat(uint32_t blockNumber, uint8_t options) {
  return cacheFetch(blockNumber, options | CACHE_STATUS_FAT_BLOCK);
}

bool SDVolume::getNextTableEntry(uint32_t cluster, uint32_t* value) {
  uint32_t lba;
  cache_t* pc;
  if (cluster < 1  || cluster > (clusterCount_ + 1)) {
    ////serial.println("Error en SDVolume cpp  cluster number fuera de rango");
    return false;
  }

  lba = fTableStartBlock_ + (cluster >> 7);

  readBlock(lba, cacheBuffer_.data);
  //blockNumber_ = lba;
  pc = &cacheBuffer_;
  if (!pc) {
    ////serial.println("Error en SDVolume cpp no pudo leer fTable sector");
    return false;
  }
  
  *value = pc->ftable[cluster & 0X7F]; //equivalente al mod!
  return true;
}

bool SDVolume::setTableEntry(uint32_t cluster, uint32_t value) {
  uint32_t lba;
  cache_t* pc;
  // error if reserved cluster of beyond FAT
  if (cluster < 1 || cluster > (clusterCount_ + 1)) {
    ////serial.println("Error en SDVolume.cpp, cluster number fuera de rango");
    goto fail;
  }

  lba = fTableStartBlock_ + (cluster >> 7);

  readBlock(lba, cacheBuffer_.data);
  blockNumber_ = lba;
  pc = &cacheBuffer_;
  if (!pc) {
    ////serial.println("Error en SDVolume.cpp, no pudo leer fTable sector");
    goto fail;
  }

  pc->ftable[cluster & 0X7F] = value;
  writeBlock(lba, cacheBuffer_.data);
  return true;

 fail:
  return false;
}