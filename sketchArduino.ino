#include <SDFFS.h>
#define SD_ChipSelectPin 10 //using digital pin 4 on arduino nano 328
#include <TMRpcm.h> // also need to include this library...
SDFFS sd;

TMRpcm tmrpcm;
char mychar;

void setup(){
  Serial.begin(9600);
  tmrpcm.speakerPin = 9; 
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  if (!sd.init(SPI_HALF_SPEED)) { // see if the card is present and can be initialized:
    Serial.println("SD fail hola"); 
    return; 
  }

  SDFile file;
  SDFile dir;
  while (dir.openNext(sd.vwd(), O_READ)){
  
    if(dir.isDir()){
      while (file.openNext(&dir, O_READ)){
        tmrpcm.play(file);
        while(tmrpcm.isPlaying());
        Serial.println("termino una cancion");
        file.close();
      }
      dir.close();
    }
  
  }

}

void loop(){}
