#include <Arduino.h>
#include <ConfigConstants.h>
#include <SDPhysical.h>
#include <SDConstants.h>
#include <SPI.h>

class SDCard {
 public:
  SDCard() : errorCode_(SD_CARD_ERROR_INIT_NOT_CALLED), type_(0) {}
  uint32_t cardSize();
  bool erase(uint32_t firstBlock, uint32_t lastBlock);
  bool eraseSingleBlockEnable();

  void error(uint8_t code) {errorCode_ = code;}
  int errorCode() const {return errorCode_;}
  int errorData() const {return status_;}

  bool initMedia(uint8_t sckRateID = SPI_FULL_SPEED, uint8_t chipSelectPin = SD_CHIP_SELECT_PIN);

  bool readBlock(uint32_t block, uint8_t* dst);

  bool readCID(cid_t* cid) {
    return readRegister(CMD10, cid);
  }

  bool readCSD(csd_t* csd) {
    return readRegister(CMD9, csd);
  }
  bool readData(uint8_t *dst);
  bool readStart(uint32_t blockNumber);
  bool readStop();
  bool setSckRate(uint8_t sckRateID);

  int type() const {return type_;}
  bool writeBlock(uint32_t blockNumber, const uint8_t* src);
  bool writeData(const uint8_t* src);
  bool writeStart(uint32_t blockNumber, uint32_t eraseCount);
  bool writeStop();

 private:

  uint8_t chipSelectPin_;
  uint8_t errorCode_;
  uint8_t spiRate_;
  uint8_t status_;
  uint8_t type_;


  uint8_t cardAcmd(uint8_t cmd, uint32_t arg) {
    sendSDCmd(CMD55, 0);
    return sendSDCmd(cmd, arg);
  }
  uint8_t sendSDCmd(uint8_t cmd, uint32_t arg);
  bool readData(uint8_t* dst, size_t count);
  bool readRegister(uint8_t cmd, void* buf);
  void disableSD();
  void enableSD();
  void type(uint8_t value) {type_ = value;}
  bool waitNotBusy(uint16_t timeoutMillis);
  bool writeData(uint8_t token, const uint8_t* src);
};