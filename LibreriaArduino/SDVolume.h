#include <SDCard.h>
#include <MyStructs.h>

union cache_t {
  uint8_t  data[512];
  uint32_t ftable[128];
  dir_t    dir[16];
  sb_t	   superblock;
};

class SDVolume {
public:
	bool init(SDCard* dev);

	uint8_t sectorsPerCluster() const {return sectorsPerCluster_;}
	uint32_t sectorsPerFTable()  const {return sectorsPerFTable_;}
	uint32_t clusterCount() const {return clusterCount_;}
	uint8_t clusterSizeShift() const {return clusterSizeShift_;}
	uint32_t dataStartBlock() const {return dataStartSector_;}
	uint32_t fTableStartBlock() const {return fTableStartBlock_;}
	uint32_t rootDirEntryCount() const {return rootDirEntryCount_;}
	uint32_t rootDirStart() const {return rootDirStart_;}
	uint32_t blockNumber() const {return blockNumber_;}
	SDCard* sdCard() {return sdCard_;}


  bool readBlock(uint32_t block, uint8_t* dst) {
  	return sdCard_->readBlock(block, dst);
  }
  bool writeBlock(uint32_t block, const uint8_t* dst) {
  	return sdCard_->writeBlock(block, dst);
  }

  uint8_t sectorOfCluster(uint32_t position) const {
      return (position >> 9) & (sectorsPerCluster_ - 1);
  }

  uint32_t clusterStartBlock(uint32_t cluster) const {
		return cluster*sectorsPerCluster_;
	}

  //-----------------------------------------------------------------
  cache_t *cacheAddress() {return &cacheBuffer_;}
  cache_t* cacheFetch(uint32_t blockNumber, uint8_t options);
  cache_t* cacheFetchFat(uint32_t blockNumber, uint8_t options);

private:
	friend class SDFile;

	SDCard* sdCard_; 
	cache_t cacheBuffer_;
	uint32_t blockNumber_;
	uint8_t sectorsPerCluster_;    
	uint32_t sectorsPerFTable_;
	uint32_t clusterCount_;
	uint8_t clusterSizeShift_;
	uint32_t dataStartSector_;
	uint32_t fTableStartBlock_;
	uint16_t rootDirEntryCount_;
	uint32_t rootDirStart_;

	bool freeChain(uint32_t cluster);
	bool getNextTableEntry(uint32_t cluster, uint32_t* value);
	bool setTableEntry(uint32_t cluster, uint32_t value);
	bool isEOC(uint32_t cluster) const {
  	return  cluster >= EOC_MIN;
	}

  //ondas para la cache
  static const uint8_t CACHE_STATUS_DIRTY = 1;
  static const uint8_t CACHE_STATUS_FAT_BLOCK = 2;
  static const uint8_t CACHE_STATUS_MASK = CACHE_STATUS_DIRTY | CACHE_STATUS_FAT_BLOCK;
  static const uint8_t CACHE_OPTION_NO_READ = 4;
  // value for option argument in cacheFetch to indicate read from cache
  static uint8_t const CACHE_FOR_READ = 0;
  // value for option argument in cacheFetch to indicate write to cache
  static uint8_t const CACHE_FOR_WRITE = CACHE_STATUS_DIRTY;
  // reserve cache block with no read
  static uint8_t const CACHE_RESERVE_FOR_WRITE = CACHE_STATUS_DIRTY | CACHE_OPTION_NO_READ;
};