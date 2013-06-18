#include <SDFile.h>
SDFile* SDFile::cwd_ = 0;

void SDFile::setpos(Pos_t* pos) {
  curPosition_ = pos->position;
  curCluster_ = pos->cluster;
}

void SDFile::getpos(Pos_t* pos) {
  pos->position = curPosition_;
  pos->cluster = curCluster_;
}

int SDFile::peek() {
  Pos_t pos;
  getpos(&pos);
  int c = read();
  if (c >= 0) setpos(&pos);
  return c;
}

bool SDFile::close(){
  type_ = FFS_FILE_TYPE_CLOSED;
	return true; //mientras FFS sea solo para lectura, implementar luego
}

dir_t* SDFile::cacheDirEntry(uint8_t action) {
  cache_t* pc;
  vol_->readBlock(dirBlock_, pc->data);
  if (!pc) {
    goto fail;
  }
  Serial.println(dirBlock_);
  Serial.println(dirIndex_);
  return pc->dir + dirIndex_;

 fail:
  return 0;
}

bool SDFile::dirEntry(dir_t* dir){
  dir_t* p;
  // make sure fields on SD are correct
  // if (!sync()) {
  //   DBG_FAIL_MACRO;
  //   goto fail;
  // }
  // read entry
  p = cacheDirEntry(SDVolume::CACHE_FOR_READ);
  //Serial.println("name");
  //Serial.println((char*)(p->name));
  ////serial.println(p->firstCluster);
  if (!p) {
    goto fail;
  }
  // copy to caller's struct
  memcpy(dir, p, sizeof(dir_t));
  return true;

 fail:
  return false;
}

void SDFile::dirName(const dir_t& dir, char* name) {
  uint8_t j = 0;
  for (uint8_t i = 0; i < 11; i++) {
    if (dir.name[i] == ' ')continue;
    if (i == 8) name[j++] = '.';
    name[j++] = dir.name[i];
  }
  name[j] = 0;
}

bool SDFile::exists(const char* name) {
  SDFile file;
  return file.open(this, name, O_READ);
}

int16_t SDFile::fgets(char* str, int16_t num, char* delim) {
  char ch;
  int16_t n = 0;
  int16_t r = -1;
  while ((n + 1) < num && (r = read(&ch, 1)) == 1) {
    // delete CR
    if (ch == '\r') continue;
    str[n++] = ch;
    if (!delim) {
      if (ch == '\n') break;
    } else {
      if (strchr(delim, ch)) break;
    }
  }
  if (r < 0) {
    // read error
    return -1;
  }
  str[n] = '\0';
  return n;
}

bool SDFile::getFilename(char* name) {
  dir_t* p;
  if (!isOpen()) {
    goto fail;
  }
  if (isRoot()) {
    name[0] = '/';
    name[1] = '\0';
    return true;
  }
  // cache entry
  dirEntry(p);

  if (!p) {
  	////serial.println("Error SDFile.cpp, dir_t not found");
    goto fail;
  }
  // format name

  dirName(*p, name);
  Serial.println((char*)(p->name));
  return true;

 fail:
  return false;
}

bool SDFile::openNext(SDFile* dirFile, uint8_t oflag) {
  dir_t* p;
  uint8_t index;
  if (!dirFile) {
    Serial.println("not dirFile");
    goto fail;
  }
  // error if already open
  if (isOpen()) {
    //close();
    goto fail;
  }
  vol_ = dirFile->vol_;

   Serial.print("current dir position ");
   Serial.println(dirFile->curPosition_ >> 5);
   Serial.println(dirFile->curCluster_);
   Serial.println();
  curCluster_ = dirFile->firstCluster_;

  while (1) {
    index = 0XF & (dirFile->curPosition_ >> 5);

    // read entry into cache
    p = dirFile->readNextDirEntry();   

    if (!p) {
      goto fail;
    }
    // done if last entry
    if (p->name[0] == DIR_NAME_FREE) {
      goto fail;
    }
    // skip empty slot or '.' or '..'
    if (p->name[0] == DIR_NAME_DELETED || p->name[0] == '.' || p->attributes == 0X5) {
      Serial.println(".");
      continue;
    }
     Serial.print("name: ");
     for(int i = 0; i< 11; i++)
         Serial.print((char)p->name[i]);
     Serial.println();
     Serial.print("attributes 0x");
     Serial.println(p->attributes, HEX);

     Serial.print("entry firstCluster ");
     Serial.println(p->firstCluster);
    // must be file or dir
    if (DIR_IS_FILE_OR_SUBDIR(*p)) {
      return openEntry(index, oflag);
    }
  }

 fail:
  return false;
}

bool SDFile::openRoot(SDVolume* vol) {
  if (isOpen()) {
    goto fail;
  }
  vol_ = vol;
 
    type_ = FFS_FILE_TYPE_ROOT;
    firstCluster_ = vol->rootDirStart();
    ////serial.print("FirstCluster de root: ");
    ////serial.println(firstCluster_); //542
    if (!setDirSize()) {
      goto fail;
    }
    
  
  flags_ = O_READ;

  curCluster_ = 0;
  curPosition_ = 0;

  // root no tiene directory entry
  dirBlock_ = 0;
  dirIndex_ = 0;
  return true;

 fail:
  return false;
}

bool SDFile::open(const char* path, uint8_t oflag) {
  char namee[11];
  namee[11] = '\0';
  //SDFile::cwd_->getFilename(namee);
  //Serial.println(namee);
  //Serial.println("---");
  return open(SDFile::cwd_, path, oflag);
}

bool SDFile::open(SDFile* dirFile, const char* path, uint8_t oflag) {
  uint8_t dname[11];
  SDFile dir1, dir2;
  SDFile *parent = dirFile;
  SDFile *sub = &dir1;

  if (!dirFile) {
    goto fail;
  }
  if (isOpen()) {
    goto fail;
  }
  if (*path == '/') {
    while (*path == '/') path++;
    if (!dirFile->isRoot()) {
      if (!dir2.openRoot(dirFile->vol_)) {
        goto fail;
      }
      parent = &dir2;
    }
  }
  if(!make83Name(path, dname, &path)){
    goto fail;
  }
  while(!*path == '/') path++;

  if(!parent->isRoot() && *path !=0){
    ////serial.println("Error SDFile.cpp, solo se permite un nivel de folder");
    goto fail;
  }
  return open(parent, dname, oflag);  

 fail:
  return false;
}

bool SDFile::open(SDFile* dirFile, uint16_t index, uint8_t oflag) {
  dir_t* p;

  vol_ = dirFile->vol_;

  // error if already open
  if (isOpen() || !dirFile) {
    ////serial.println("Error SDFile.cpp, open con index error");
    goto fail;
  }

  // seek to location of entry
  if (!dirFile->seekSet(32 * index)) {
    ////serial.println("Error SDFile.cpp, seekSet fallo");
    goto fail;
  }
  // read entry into cache
  p = dirFile->readNextDirEntry();
  if (!p) {
    ////serial.println("Error SDFile.cpp, readNextDirEntry fallo");
    goto fail;
  }
  // error if empty slot or '.' or '..'
  if (p->name[0] == DIR_NAME_FREE ||
      p->name[0] == DIR_NAME_DELETED || p->name[0] == '.') {
    goto fail;
  }

  return openEntry(index & 0XF, oflag);

 fail:
  return false;
}

dir_t* SDFile::readNextDirEntry() {
  uint8_t i;
  // error if not directory
  if (!isDir()) {
    goto fail;
  }
  // index of entry in cache
  i = (curPosition_ >> 5) & 0XF;

  // use read to locate and cache block
  if (read() < 0) {
    goto fail;
  }
  // //serial.print("Cur pos ");
  // //serial.println(curPosition_);

  // //serial.print("first cluster ");
  // //serial.println(firstCluster_);

  // //serial.print("Cur cluster ");
  // //serial.println(curCluster_);

  // //serial.print("blockNumber ");
  // //serial.println(vol_->blockNumber());

  cache_t *t;
  t = vol_->cacheFetch(curCluster_*8, SDVolume::CACHE_FOR_READ);
  

  // advance to next entry
  curPosition_ += 31;


  // return pointer to entry
  return &(t->dir[i]);

 fail:
  return 0;
}

bool SDFile::open(SDFile* dirFile, const uint8_t dname[11], uint8_t oflag) {
  cache_t* pc;
  uint8_t i;

  bool emptyFound = false;
  bool fileFound = false;  
  dir_t* p;
  
  if(dirFile->isFile()) goto fail;

  vol_ = dirFile->vol_;

  //porque hay que buscar secuencialmente
  dirFile->rewind();
  if(dirFile->isRoot()){
    Serial.print("root firstCluster: ");
    Serial.println(dirFile->firstCluster_);
  }else{
    Serial.print("open de NO root: ");
    Serial.println(dirFile->firstCluster_);
  }

  while (dirFile->curPosition_ < dirFile->fileSize_) {
    i = (dirFile->curPosition_ >> 5) & 0XF;
    p = dirFile->readNextDirEntry();

    if (!p) {
      goto fail;
    }
    if(p->name[0] == DIR_NAME_FREE || p->name[0] == DIR_NAME_DELETED){
      if (!emptyFound) {
        emptyFound = true;
        dirBlock_ = vol_->blockNumber();
        dirIndex_ = i;        
      }
      // done if no entries follow
      if (p->name[0] == DIR_NAME_FREE) break;
    }else{
      if(memcmp(p->name, dname, 11)==0){
        fileFound = true;
        break;
      }
    }
  }
  return openEntry(i, oflag);  

 fail:
  return false;
}

bool SDFile::openEntry(uint8_t dirIndex, uint8_t oflag) {
  dir_t* p = &vol_->cacheFetch(curCluster_*8, SDVolume::CACHE_FOR_READ)->dir[dirIndex];

  // remember location of directory entry on SD
  dirBlock_ = vol_->blockNumber();
  dirIndex_ = dirIndex;

  // copy first cluster number for directory fields
  firstCluster_ = p->firstCluster;

  // make sure it is a normal file or subdirectory
  if (DIR_IS_FILE(*p)) {
    fileSize_ = p->fileSize;
    type_ = FFS_FILE_TYPE_NORMAL;
  }else if (DIR_IS_SUBDIR(*p)) {
    if (!setDirSize()) {
      goto fail;
    }
    type_ = FFS_FILE_TYPE_SUBDIR;
  }else {
    goto fail;
  }

  flags_ = oflag;

  // set to start of file
  curCluster_ = 0;
  curPosition_ = 0;

  return true;

 fail:
  type_ = FFS_FILE_TYPE_CLOSED;
  return false;
}

bool SDFile::make83Name(const char* str, uint8_t* name, const char** ptr) {
  uint8_t c;
  uint8_t n = 7;  
  uint8_t i = 0;

  while (i < 11) name[i++] = 0;

  i = 0;
  while (*str != '\0' && *str != '/') {
    c = *str++;
    if (c == '.') {
      while(i < 8) name[i++] = ' ';
      if (n == 10) {
        goto fail;
      }
      n = 10; 
      i = 8;   
    } else {

#ifdef __AVR__
      // store chars in flash
      PGM_P p = PSTR("|<>^+=?/[];,*\"\\");
      uint8_t b;
      while ((b = pgm_read_byte(p++))) if (b == c) {
        goto fail;
      }
#else  // __AVR__
      // store chars in RAM
      if (strchr("|<>^+=?/[];,*\"\\", c)) {
        goto fail;
      }
#endif  // __AVR__

      if (i > n || c < 0X21 || c > 0X7E) {
        goto fail;
      }
      // convert lower to upper
      name[i++] = c < 'a' || c > 'z' ?  c : c + ('A' - 'a');
    }
  }
  *ptr = str;
  return name[0] != ' ' && name[0] != 0;

 fail:
  return false;
}

bool SDFile::seekSet(uint32_t pos) {
  uint32_t nCur;
  uint32_t nNew;

  if (!isOpen() || pos > fileSize_) {
    ////serial.println("Error SDFile, archivo no esta abierto");
    goto fail;
  }

  if (pos == 0) {
    // set position to start of file
    curCluster_ = 0;
    curPosition_ = 0;
    goto done;
  }
  // calculate cluster index for cur and new position
  nCur = (curPosition_ - 1) >> (vol_->clusterSizeShift_ + 9);
  nNew = (pos - 1) >> (vol_->clusterSizeShift_ + 9);

  if (nNew < nCur || curPosition_ == 0) {
    // must follow chain from first cluster
    curCluster_ = firstCluster_;
  } else {
    // advance from curPosition
    nNew -= nCur;
  }
  while (nNew--) {
    if (!vol_->getNextTableEntry(curCluster_, &curCluster_)) {
      ////serial.println("Error SDFile.cpp, fallo getNextTableEntry");
      goto fail;
    }
  }
  curPosition_ = pos;

 done:
  return true;

 fail:
  return false;
}

bool SDFile::setDirSize() {
  // Serial.print("setDirSize filename: ");
  // char n[11];
  // getFilename(n);
  // Serial.println(n);
  uint16_t s = 0;
  uint32_t cluster = firstCluster_;
  do {
    if (!vol_->getNextTableEntry(cluster, &cluster)) {
      
      goto fail;
    }
    // Serial.print("next cluster ");      
    // Serial.println(cluster);
    s += vol_->sectorsPerCluster();
    if (s >= 4096) {
      goto fail;
    }
  } while (!vol_->isEOC(cluster));
  fileSize_ = 512L*s;
  return true;

 fail:
  return false;
}

int16_t SDFile::read() {
  uint8_t b;
  return read(&b, 1) == 1 ? b : -1;
}

int SDFile::read(void* buf, size_t nbyte) {
  uint8_t sectorOfCluster;
  uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
  uint16_t offset;
  size_t toRead;
  uint32_t block;  // raw device block number
  cache_t* pc;

  // error if not open or write only
  if (!isOpen() || !(flags_ & O_READ)) {
    ////serial.println("Error SDFile.cpp, archivo no esta abierto(2)");
    goto fail;
  }
  // max bytes left in file
  if (nbyte >= (fileSize_ - curPosition_)) {
    Serial.println("ultimo byte");
    Serial.println(curPosition_);
    nbyte = fileSize_ - curPosition_;
  }
  // amount left to read
  toRead = nbyte;
   // Serial.print("dentro de read...");
   // Serial.print(nbyte);
   // Serial.println(" bytes");
  while (toRead > 0) {
    size_t n;
    offset = curPosition_ & 0X1FF;  // offset in block
    sectorOfCluster = vol_->sectorOfCluster(curPosition_);
    ////serial.print("firstCluster ");
    ////serial.println(firstCluster_);

    if (offset == 0 && sectorOfCluster == 0) {
      if (curPosition_ == 0) {
        curCluster_ = firstCluster_;
      } else {
        if (!vol_->getNextTableEntry(curCluster_, &curCluster_)) {
          goto fail;
        }
      }
    }
    block = vol_->clusterStartBlock(curCluster_) + sectorOfCluster;
    
    if (offset != 0 || toRead < 512 || block == vol_->blockNumber()) {
      // amount to be read from current block
      ////serial.println("dst2");
      n = 512 - offset;
      if (n > toRead) n = toRead;
      // read block to cache and copy data to caller
      //vol_->readBlock(block, dst);
      pc = vol_->cacheFetch(block, SDVolume::CACHE_FOR_READ);
      if (!pc) {
        ////serial.println("Error SDFile.cpp, luego de cacheAddress");
        goto fail;
      }
      uint8_t* src = pc->data + offset;
      memcpy(dst, src, n);
    } else if (toRead < 1024) {
      // read single block
      n = 512;
      if (!vol_->readBlock(block, dst)) {
        ////serial.println("Error SDFile.cpp, luego del readBlock");
        goto fail;
      }
    } else {
     // //serial.println("dst3");
      uint8_t nb = toRead >> 9;
      n = 512*nb;

      if (!vol_->sdCard()->readStart(block)) {
        goto fail;
      }
      for (uint8_t b = 0; b < nb; b++) {
        if (!vol_->sdCard()->readData(dst + b*512)) {
          goto fail;
        }
      }
      if (!vol_->sdCard()->readStop()) {
        goto fail;
      }
    }
    dst += n;
    curPosition_ += n;
    toRead -= n;
  }
  return nbyte;

 fail:
  return -1;
}

int8_t SDFile::readDir(dir_t* dir) {
  int16_t n;
  // if not a directory file or miss-positioned return an error
  if (!isDir() || (0X1F & curPosition_)) return -1;

  while (1) {
    n = read(dir, sizeof(dir_t));
    if (n != sizeof(dir_t)) return n == 0 ? 0 : -1;
    // last entry if DIR_NAME_FREE
    if (dir->name[0] == DIR_NAME_FREE) return 0;
    // skip empty entries and entry for .  and ..
    if (dir->name[0] == DIR_NAME_DELETED || dir->name[0] == '.') continue;
    // return if normal file or subdirectory
    if (DIR_IS_FILE_OR_SUBDIR(*dir)) return n;
  }
}