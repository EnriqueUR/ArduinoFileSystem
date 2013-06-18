#include <SDFile.h>

class SDFFS {
 public:
  SDFFS() {}

  bool init(uint8_t sckRateID = SPI_FULL_SPEED, uint8_t chipSelectPin = SD_CHIP_SELECT_PIN) {
    return begin(chipSelectPin, sckRateID);
  }

  bool chdir(bool set_cwd = false);
  //bool chdir(const char* path, bool set_cwd = false);
  void chvol();

  bool exists(const char* name);
  bool begin(uint8_t chipSelectPin = SD_CHIP_SELECT_PIN, uint8_t sckRateID = SPI_FULL_SPEED);

  SDCard* card() {return &card_;}
  SDVolume* vol() {return &vol_;}
  SDFile* vwd() {return &vwd_;}

 private:
  SDCard card_;
  SDVolume vol_;
  SDFile vwd_;
};