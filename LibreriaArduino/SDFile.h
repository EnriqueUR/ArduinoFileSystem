#ifdef __AVR__
#include <avr/pgmspace.h>
#else  // __AVR__
#ifndef PGM_P
/** pointer to flash for ARM */
#define PGM_P const char*
#endif  // PGM_P
#ifndef PSTR
/** store literal string in flash for ARM */
#define PSTR(x) (x)
#endif  // PSTR
#ifndef pgm_read_byte
/** read 8-bits from flash for ARM */
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#endif  // pgm_read_byte
#ifndef pgm_read_word
/** read 16-bits from flash for ARM */
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#endif  // pgm_read_word
#ifndef PROGMEM
/** store in flash for ARM */
#define PROGMEM const
#endif  // PROGMEM
#endif  // __AVR__


#ifndef SDFile_h
#define SDFile_h

#include <Arduino.h>
#include <SDVolume.h>

struct Pos_t {
  /** stream position */
  uint32_t position;
  /** cluster for position */
  uint32_t cluster;
  Pos_t() : position(0), cluster(0) {}
};

class SDFile {
 public:
  SDFile() :type_(FFS_FILE_TYPE_CLOSED) {}
  SDFile(const char* path, uint8_t oflag);
  ~SDFile() {if(isOpen()) close();}

  void getpos(Pos_t* pos);
  void setpos(Pos_t* pos);

  uint32_t available() {return fileSize() - curPosition();}
  bool close();
  uint32_t curCluster() const {return curCluster_;}
  uint32_t curPosition() const {return curPosition_;}
  static SDFile* cwd() {return cwd_;}

  bool dirEntry(dir_t* dir);
  static void dirName(const dir_t& dir, char* name);
  bool getFilename(char* name);

  bool exists(const char* name);
  int16_t fgets(char* str, int16_t num, char* delim = 0);

  uint32_t fileSize() const {return fileSize_;}
  uint32_t firstCluster() const {return firstCluster_;}

  bool isDir() const {return type_ >= FFS_FILE_TYPE_MIN_DIR;}
  bool isFile() const {return type_ == FFS_FILE_TYPE_NORMAL;}
  bool isOpen() const {return type_ != FFS_FILE_TYPE_CLOSED;}
  bool isRoot() const {return type_ == FFS_FILE_TYPE_ROOT;}

  dir_t* readNextDirEntry();
  bool open(SDFile* dirFile, uint16_t index, uint8_t oflag);
  bool open(SDFile* dirFile, const char* path, uint8_t oflag);
  bool open(const char* path, uint8_t oflag = O_READ);
  bool openEntry(uint8_t dirIndex, uint8_t oflag);
  bool openNext(SDFile* dirFile, uint8_t oflag);
  bool openRoot(SDVolume* vol);
  int peek();

  int16_t read();
  int read(void* buf, size_t nbyte);
  int8_t readDir(dir_t* dir);

  void rewind() {seekSet(0);}

  bool seekCur(int32_t offset) { return seekSet(curPosition_ + offset);}
  bool seekEnd(int32_t offset = 0) {return seekSet(fileSize_ + offset);}
  bool seekSet(uint32_t pos);

  uint8_t type() const {return type_;}
  SDVolume* volume() const {return vol_;}

  dir_t* cacheDirEntry(uint8_t action);

//------------------------------------------------------------------------------
 private:
  friend class SDFFS;
  static SDFile* cwd_;

  // private data
  uint8_t   flags_;         // See above for definition of flags_ bits
  uint8_t   fstate_;        // error and eof indicator
  uint8_t   type_;          // type of file see above for values
  uint8_t   dirIndex_;      // index of directory entry in dirBlock
  SDVolume* vol_;           // volume where file is located
  uint32_t  curCluster_;    // cluster for current file position
  uint32_t  curPosition_;   // current file position in bytes from beginning
  uint32_t  dirBlock_;      // block for this files directory entry
  uint32_t  fileSize_;      // file size in bytes
  uint32_t  firstCluster_;  // first cluster of file

  static bool make83Name(const char* str, uint8_t* name, const char** ptr);
  bool open(SDFile* dirFile, const uint8_t dname[11], uint8_t oflag);
  bool setDirSize();

  //--------------------------------------------------------------------
  

};

#endif