#include <SDFFS.h>

bool SDFFS::begin(uint8_t chipSelectPin, uint8_t sckRateID) {
	////serial.println("MIERDA");
	if(!card_.initMedia(sckRateID, chipSelectPin)){
		////serial.println("Error dentro de begin1 en SDFFS.cpp");
		return false;
	}
	if(!vol_.init(&card_)){
		////serial.println("Error dentro de begin2 en SDFFS.cpp");
		return false;
	}
	if(!chdir(true)){
		////serial.println("Error dentro de begin3 en SDFFS.cpp");
		return false;
	}return true;
}

bool SDFFS::chdir(bool set_cwd) {
  if (set_cwd) SDFile::cwd_ = &vwd_;
  if (vwd_.isOpen()) vwd_.close();
  return vwd_.openRoot(&vol_);
}

void SDFFS::chvol() {
  SDFile::cwd_ = &vwd_;
}

bool SDFFS::exists(const char* name) {
  return vwd_.exists(name);
}