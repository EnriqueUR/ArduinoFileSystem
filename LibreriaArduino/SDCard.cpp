#include <SDCard.h>

#define SD_TRACE(m, b) ////serial.print(m);////serial.println(b);

// SPI functions
//==============================================================================
#if (SPR0 != 0 || SPR1 != 1)
#error unexpected SPCR bits
#endif  
//------------------------------------------------------------------------------
/**
 * initialize SPI pins
 */
static void spiBegin() {
  // set SS high - may be chip select for another SPI device
  digitalWrite(SS, HIGH);
  // SS must be in output mode even it is not chip select
  pinMode(SS, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
}
//------------------------------------------------------------------------------
/**
 * Initialize hardware SPI
 * Set SCK rate to F_CPU/pow(2, 1 + spiRate) for spiRate [0,6]
 */
static void spiInit(uint8_t spiRate) {
  spiRate = spiRate > 12 ? 6 : spiRate/2;
  // See avr processor documentation
  SPCR = (1 << SPE) | (1 << MSTR) | (spiRate >> 1);
  SPSR = spiRate & 1 || spiRate == 6 ? 0 : 1 << SPI2X;
}
//------------------------------------------------------------------------------
/** SPI receive a byte */
static  uint8_t readSPI() {
  SPDR = 0XFF;
  while (!(SPSR & (1 << SPIF)));
  return SPDR;
}
//------------------------------------------------------------------------------
/** SPI receive multiple bytes */
static uint8_t readSPI(uint8_t* buf, size_t n) {
  if (n-- == 0) return 0;
  SPDR = 0XFF;
  for (size_t i = 0; i < n; i++) {
    while (!(SPSR & (1 << SPIF)));
    uint8_t b = SPDR;
    SPDR = 0XFF;
    buf[i] = b;
  }
  while (!(SPSR & (1 << SPIF)));
  buf[n] = SPDR;
  return 0;
}
//------------------------------------------------------------------------------
/** SPI send a byte */
static void writeSPI(uint8_t b) {
  SPDR = b;
  while (!(SPSR & (1 << SPIF)));
}
//------------------------------------------------------------------------------
static void writeSPI(const uint8_t* buf , size_t n) {
  if (n == 0) return;
  SPDR = buf[0];
  if (n > 1) {
    uint8_t b = buf[1];
    size_t i = 2;
    while (1) {
      while (!(SPSR & (1 << SPIF)));
      SPDR = b;
      if (i == n) break;
      b = buf[i++];
    }
  }
  while (!(SPSR & (1 << SPIF)));
}

#define clockSPI() writeSPI(0XFF);
//==============================================================================
// SDCard member functions
//------------------------------------------------------------------------------
// send command and return error code.  Return zero for OK
uint8_t SDCard::sendSDCmd(uint8_t cmd, uint32_t arg) {
  enableSD();
  waitNotBusy(300);
  writeSPI(cmd | 0x40);
  writeSPI(arg >> 24);
  writeSPI(arg >> 16);
  writeSPI(arg >> 8);
  writeSPI(arg);

  writeSPI(0X95);

  for(int i = 0; i < 8; i++){
    status_ = readSPI();
    if( status_ != 0XFF) 
      break;
  }
  return status_;
}
//------------------------------------------------------------------------------
void SDCard::disableSD() {
  digitalWrite(chipSelectPin_, HIGH);
  clockSPI();
}
//------------------------------------------------------------------------------
void SDCard::enableSD() {
  spiInit(spiRate_);
  digitalWrite(chipSelectPin_, LOW);
}

//Retorna la cantidad de bloques de 512 bytes total!
uint32_t SDCard::cardSize() {
  csd_t csd;
  if (!readCSD(&csd)) return 0;
  if (csd.v1.csd_ver == 0) {
    uint8_t read_bl_len = csd.v1.read_bl_len;
    uint16_t c_size = (csd.v1.c_size_high << 10)
                      | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
    uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1)
                          | csd.v1.c_size_mult_low;
    return (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
  } else if (csd.v2.csd_ver == 1) {
    uint32_t c_size = 0X10000L * csd.v2.c_size_high + 0X100L
                      * (uint32_t)csd.v2.c_size_mid + csd.v2.c_size_low;
    return (c_size + 1) << 10;
  } else {
    error(SD_CARD_ERROR_BAD_CSD);
    return 0;
  }
}

bool SDCard::erase(uint32_t firstBlock, uint32_t lastBlock) {
  csd_t csd;
  if (!readCSD(&csd)) goto fail;
  // check for single block erase
  if (!csd.v1.erase_blk_en) {
    // erase size mask
    uint8_t m = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
    if ((firstBlock & m) != 0 || ((lastBlock + 1) & m) != 0) {
      // error card can't erase specified area
      error(SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
      goto fail;
    }
  }
  if (type_ != SD_CARD_TYPE_SDHC) {
    firstBlock <<= 9;
    lastBlock <<= 9;
  }
  if (sendSDCmd(CMD32, firstBlock)
    || sendSDCmd(CMD33, lastBlock)
    || sendSDCmd(CMD38, 0)) {
      error(SD_CARD_ERROR_ERASE);
      goto fail;
  }
  if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
    error(SD_CARD_ERROR_ERASE_TIMEOUT);
    goto fail;
  }
  disableSD();
  return true;

 fail:
  disableSD();
  return false;
}

bool SDCard::eraseSingleBlockEnable() {
  csd_t csd;
  return readCSD(&csd) ? csd.v1.erase_blk_en : false;
}

bool SDCard::initMedia(uint8_t sckRateID, uint8_t chipSelectPin) {
  errorCode_ = type_ = 0;
  chipSelectPin_ = chipSelectPin;

  spiBegin();

  uint16_t  t0 = (uint16_t)millis();
  uint32_t arg;

  pinMode(chipSelectPin_, OUTPUT);

// set SCK rate for initialization commands
  spiRate_ = SPI_SD_INIT_RATE;
  spiInit(spiRate_);

  disableSD();

  // must supply min of 74 clock cycles with CS high.
  for (uint8_t i = 0; i < 10; i++) 
    clockSPI();

  enableSD();

  uint8_t r ;
  //disableSD();
  while(sendSDCmd(CMD0, 0) != R1_IDLE_STATE){
    //error(SD_CARD_ERROR_CMD0);
    //goto fail;
  }
  
  //digitalWrite(chipSelectPin_, HIGH);
  uint8_t c;
  c = sendSDCmd(0X01, 0);
  //digitalWrite(chipSelectPin_, HIGH);
  
  t0 = (uint16_t)millis();
  while (1) {
    r = sendSDCmd(0X01, 0);
    disableSD();
    if(!r)
      break;

    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_ACMD41);
      ////serial.print("Error aqui 2");
      goto fail;
    }
  }
  return true;

  fail:
    return false;
}

bool SDCard::readBlock(uint32_t blockNumber, uint8_t* dst) {
  if (sendSDCmd(CMD17, blockNumber << 9)) {
    error(SD_CARD_ERROR_CMD17);
    goto fail;
  }

  uint16_t t0;
  t0 = millis();
  uint8_t r;
  while(1){
    r = readSPI();
    if(r == DATA_START_BLOCK)
      break;
    if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) {
      error(SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }

  for(int i = 0; i < 512; i++){
    dst[i] = readSPI();
  }
  readSPI();
  readSPI();

  return (r == DATA_START_BLOCK);

 fail:
  disableSD();
  return false;
}

//Hasta aqui conforme al pdf del inge

bool SDCard::readData(uint8_t *dst) {
  enableSD();
  return readData(dst, 512);
}
//------------------------------------------------------------------------------
bool SDCard::readData(uint8_t* dst, size_t count) {
  uint16_t crc;
  // wait for start block token
  uint16_t t0 = millis();
  while ((status_ = readSPI()) == 0XFF) {
    if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) {
      error(SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }
  if (status_ != DATA_START_BLOCK) {
    error(SD_CARD_ERROR_READ);
    goto fail;
  }
  // transfer data
  if (status_ = readSPI(dst, count)) {
    error(SD_CARD_ERROR_SPI_DMA);
    goto fail;
  }
  readSPI();
  readSPI();

  disableSD();
  return true;

 fail:
  disableSD();
  return false;
}

bool SDCard::readRegister(uint8_t cmd, void* buf) {
  uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
  uint8_t retVal;
  if (retVal = sendSDCmd(cmd, 0)) {
    error(retVal); //aki iba SD_CARD_ERROR_READ_REG
    goto fail;
  }
  return readData(dst, 16);

 fail:
  disableSD();
  return false;
}

bool SDCard::readStart(uint32_t blockNumber) {
  SD_TRACE("RS", blockNumber);
  if (type()!= SD_CARD_TYPE_SDHC) blockNumber <<= 9;
  if (sendSDCmd(CMD18, blockNumber)) {
    error(SD_CARD_ERROR_CMD18);
    goto fail;
  }
  disableSD();
  return true;

 fail:
  disableSD();
  return false;
}

bool SDCard::readStop() {
  enableSD();
  if (sendSDCmd(CMD12, 0)) {
    error(SD_CARD_ERROR_CMD12);
    goto fail;
  }
  disableSD();
  return true;

 fail:
  disableSD();
  return false;
}
//------------------------------------------------------------------------------
/**
 * Set the SPI clock rate.
 *
 * \param[in] sckRateID A value in the range [0, 14].
 *
 * The SPI clock divisor will be set to approximately
 *
 * (2 + (sckRateID & 1)) << ( sckRateID/2)
 *
 * The maximum SPI rate is F_CPU/2 for \a sckRateID = 0 and the rate is
 * F_CPU/128 for \a scsRateID = 12.
 *
 * \return The value one, true, is returned for success and the value zero,
 * false, is returned for an invalid value of \a sckRateID.
 */
bool SDCard::setSckRate(uint8_t sckRateID) {
  if (sckRateID > MAX_SCK_RATE_ID) {
    error(SD_CARD_ERROR_SCK_RATE);
    return false;
  }
  spiRate_ = sckRateID;
  return true;
}
//------------------------------------------------------------------------------
// wait for card to go not busy
bool SDCard::waitNotBusy(uint16_t timeoutMillis) {
  uint16_t t0 = millis();
  while (readSPI() != 0XFF) {
    if (((uint16_t)millis() - t0) >= timeoutMillis) goto fail;
  }
  return true;

 fail:
  return false;
}
//------------------------------------------------------------------------------
/**
 * Writes a 512 byte block to an SD card.
 *
 * \param[in] blockNumber Logical block to be written.
 * \param[in] src Pointer to the location of the data to be written.
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
bool SDCard::writeBlock(uint32_t blockNumber, const uint8_t* src) {
  SD_TRACE("WB", blockNumber);
  if (sendSDCmd(CMD24, blockNumber << 9)) {
    error(SD_CARD_ERROR_CMD24);
    goto fail;
  }
  writeSPI(DATA_START_BLOCK);
  for(int i = 0 ; i< 512 ; i++){
    writeSPI(src[i]);
  }

  /*
  if(!writeData(DATA_START_BLOCK,src)){
    ////serial.println("Error en writeblock");
    goto fail;
  }
  */

  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    ////serial.println("ERROR 2");
    error(SD_CARD_ERROR_WRITE_TIMEOUT);
    goto fail;
  }
  // response is r2 so get and check two bytes for nonzero
  if (sendSDCmd(CMD13, 0) || readSPI()) {
    ////serial.println("ERROR 3");
    error(SD_CARD_ERROR_WRITE_PROGRAMMING);
    goto fail;
  }

  disableSD();
  return true;

 fail:
  disableSD();
  return false;
}


bool SDCard::writeData(const uint8_t* src) {
  enableSD();
  // wait for previous write to finish
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  if (!writeData(WRITE_MULTIPLE_TOKEN, src)) goto fail;
  disableSD();
  return true;

 fail:
  error(SD_CARD_ERROR_WRITE_MULTIPLE);
  disableSD();
  return false;
}
//------------------------------------------------------------------------------
// send one block of data for write block or write multiple blocks
bool SDCard::writeData(uint8_t token, const uint8_t* src) {
  writeSPI(token);
  writeSPI(src, 512);
  

  writeSPI(0X95 >> 8);
  writeSPI(0X95);

 // writeSPI(crc >> 8);
 // writeSPI(0XFF);

  status_ = readSPI();
  if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
    ////serial.println("Error1");
    error(SD_CARD_ERROR_WRITE);
    goto fail;
  }
  uint16_t t0;
  t0 = millis();
  while (1) {
    if(readSPI() != 0)break;
    if (((uint16_t)millis() - t0) >= SD_WRITE_TIMEOUT) goto fail;
  }
  return true;

 fail:
  disableSD();
  return false;
}
//------------------------------------------------------------------------------
/** Start a write multiple blocks sequence.
 *
 * \param[in] blockNumber Address of first block in sequence.
 * \param[in] eraseCount The number of blocks to be pre-erased.
 *
 * \note This function is used with writeData() and writeStop()
 * for optimized multiple block writes.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
bool SDCard::writeStart(uint32_t blockNumber, uint32_t eraseCount) {
  SD_TRACE("WS", blockNumber);
  // send pre-erase count
  if (cardAcmd(ACMD23, eraseCount)) {
    error(SD_CARD_ERROR_ACMD23);
    goto fail;
  }
  if (sendSDCmd(CMD25, blockNumber << 9)) {
    error(SD_CARD_ERROR_CMD25);
    goto fail;
  }
  disableSD();
  return true;

 fail:
  disableSD();
  return false;
}
//------------------------------------------------------------------------------
/** End a write multiple blocks sequence.
 *
* \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
bool SDCard::writeStop() {
  enableSD();
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  writeSPI(STOP_TRAN_TOKEN);
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  disableSD();
  return true;

 fail:
  error(SD_CARD_ERROR_STOP_TRAN);
  disableSD();
  return false;
}
