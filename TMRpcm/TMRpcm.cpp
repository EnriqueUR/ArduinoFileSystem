/*Library by TMRh20 2012-2013*/

//#include <SD.h>
#include <SDFFS.h>
#include <TMRpcm.h>
#include <mcpDac.h>
#include <ArduinoPins.h>

const int buffSize = 150; //note: there are 2 sound buffers. This will require (soundBuff*2) memory free
volatile byte buffer[2][buffSize+1];
volatile boolean buffEmpty[2] = {false,false}, whichBuff = false;
volatile unsigned int buffCount = 0,  loadCounter=0;
unsigned int volModMax = 1,volMod=2, tt=0;
boolean paused = 0, playing = 0;
volatile unsigned int resolution = 500;
/*
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
	volatile byte *TIMSK[] = {&TIMSK1,&TIMSK3,&TIMSK4,&TIMSK5};
	volatile byte *TCCRnA[] = {&TCCR1A,&TCCR3A,&TCCR4A,&TCCR5A};
	volatile byte *TCCRnB[] = {&TCCR1B, &TCCR3B,&TCCR4B,&TCCR5B};
	volatile unsigned int *OCRnA[] = {&OCR1A,&OCR3A,&OCR4A,&OCR5A};
	volatile unsigned int *OCRnB[] = {&OCR1B, &OCR3B,&OCR4B,&OCR5B};
	volatile unsigned int *ICRn[]	= {&ICR1, &ICR3,&ICR4,&ICR5};

	ISR_ALIAS(TIMER3_OVF_vect, TIMER1_OVF_vect);
	ISR_ALIAS(TIMER3_CAPT_vect, TIMER1_CAPT_vect);
	ISR_ALIAS(TIMER4_OVF_vect, TIMER1_OVF_vect);
	ISR_ALIAS(TIMER4_CAPT_vect, TIMER1_CAPT_vect);
	ISR_ALIAS(TIMER5_OVF_vect, TIMER1_OVF_vect);
	ISR_ALIAS(TIMER5_CAPT_vect, TIMER1_CAPT_vect);
#else
*/
	volatile byte *TIMSK[] = {&TIMSK1};
	volatile byte *TCCRnA[] = {&TCCR1A};
	volatile byte *TCCRnB[] = {&TCCR1B};
	volatile unsigned int *OCRnA[] = {&OCR1A};
	volatile unsigned int *OCRnB[] = {&OCR1B};
	volatile unsigned int *ICRn[]	= {&ICR1};
//#endif

void TMRpcm::setPin(){

	disable();
	pinMode(speakerPin,OUTPUT);
	switch(speakerPin){
		case 5: tt=1; break; //use TIMER3
		case 6: tt=2; break;//use TIMER4
		case 46:tt=3; break;//use TIMER5
		default:tt=0; break; //useTIMER1 as default
	}
}

SDFile sdFile;

void TMRpcm::play(SDFile myFile){
  sdFile = myFile;
  if(speakerPin != lastSpeakPin){ setPin(); lastSpeakPin=speakerPin;}
  stopPlayback();

  if(!wavInfo(myFile) ){ return; }//verify its a valid wav file
  if(myFile.isOpen()){
    
  playing = 1;
    myFile.seekSet(44); //skip the header info
    for(int i=0; i<buffSize; i++){ buffer[0][i] = myFile.read(); }
    whichBuff = false; buffEmpty[0] = false; buffEmpty[1] = true;


    //if(pwmMode){resolution = 8;}else{resolution = 4;}
    resolution = 8;
    resolution = resolution * (1000000/SAMPLE_RATE); //Serial.println(resolution);
    volModMax = (resolution*1.5) / 248 ;
    volMod = constrain(volMod, 1, volModMax);

    //noInterrupts();
    *ICRn[tt] = resolution;
    *OCRnA[tt] = *OCRnB[tt] = 1;
     //if(pwmMode){

       *TCCRnA[tt] = _BV(WGM11) | _BV(COM1A1) | _BV(COM1B0) | _BV(COM1B1); //WGM11,12,13 all set to 1 = fast PWM/w ICR TOP
       *TCCRnB[tt] = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
    //  }else{
    //   Serial.println("A punto de comenzar a reproducir");
    //   *TCCRnA[tt] = _BV(COM1A1) | _BV(COM1B0) | _BV(COM1B1);
    //   *TCCRnB[tt] = _BV(WGM13) | _BV(CS10);
    // }
    *TIMSK[tt] = ( _BV(ICIE1) | _BV(TOIE1) );
    //interrupts();

  }else{Serial.println("Read fail"); }
}



void TMRpcm::pause(){
	paused = !paused;
}


void TMRpcm::volume(int upDown){

  if(upDown){
	  volMod++; volMod = constrain(volMod, 1, volModMax);
  }else{
	  volMod--; volMod = constrain(volMod, 1, volModMax);
  }

}

boolean TMRpcm::wavInfo(SDFile myFile){
  sdFile = myFile;
  char nom[13];
  if(!myFile.isOpen() ){return 0;}
  //Serial.println("MIERDAAAAA");
  myFile.seekSet(8); String wavStr = "";
    for(int i=0; i<3; i++){ wavStr += char(myFile.read()); }
    if(!wavStr.equalsIgnoreCase("WAV")){
      Serial.println("Not a WAV file");
      //Serial.println(wavStr);
      myFile.close(); return 0;
    }

    //make sure the Sample rate is below 22000
    myFile.seekSet(24);
    unsigned int dVar = myFile.read();
     // Serial.println("dVar ");
     // Serial.println(dVar, BIN);
    dVar = myFile.read() << 8 | dVar; //read 8bit values into 16-bit integer

     // Serial.println("dVar2 ");
     // Serial.println(dVar, BIN);
    if(dVar > 22000){
      Serial.print(" SampleRate Too High: ");
      SAMPLE_RATE = 22000;
    }else{
    SAMPLE_RATE = dVar; // Set the sample rate according to the file
    }

    //verify that Bits Per Sample is 8 (0-255)
    myFile.seekSet(34); dVar = myFile.read();
     // Serial.println("dVar3 ");
     // Serial.println(dVar, BIN);
    dVar = myFile.read() << 8 | dVar;
     // Serial.println("dVar4 ");
     // Serial.println(dVar, BIN);
    if(dVar != 8){Serial.print("Wrong BitRate"); return 0;}
    return 1;
}

ISR(TIMER1_CAPT_vect){

  // The first step is to disable this interrupt before manually enabling global interrupts.
  // This allows this interrupt vector (COMPB) to continue loading data while allowing the overflow interrupt
  // to interrupt it. ( Nested Interrupts )

  // TIMSK1 &= ~_BV(ICIE1);

 //Now enable global interupts before this interrupt is finished, so the music can interrupt the buffering
  //sei();

  if(sdFile.available() < buffSize){
  	playing = 0;
    if(sdFile.isOpen()){sdFile.close();}
  	  *TIMSK[tt] &= ~( _BV(ICIE1) | _BV(TOIE1) );
  	  *OCRnA[tt] = 10;
	  *OCRnB[tt] = resolution-10;
  }else

  for(int a=0; a<2; a++){
	  if(buffEmpty[a]){
		*TIMSK[tt] &= ~(_BV(ICIE1));
	  	sei();
	  	for(int i=0; i<buffSize; i++){ buffer[a][i] = sdFile.read();	}
	    buffEmpty[a] = 0;
	}
  }

  if(paused){*TIMSK[tt] = _BV(ICIE1); *OCRnA[tt] = 10; *OCRnB[tt] = resolution-10; *TIMSK[tt] &= ~_BV(TOIE1); } //if pausedd, disable overflow vector and leave this one enabled
  else
  if(playing){
  //re-enable this interrupt vector and the overflow vector
  *TIMSK[tt] = ( _BV(ICIE1) | _BV(TOIE1) );
  }
}


ISR(TIMER1_OVF_vect){

  ++loadCounter;
  if(loadCounter == 1){ return; }

  loadCounter = 0;
  *OCRnA[tt] = *OCRnB[tt] = min(buffer[whichBuff][buffCount]*volMod,resolution);
  buffCount++;

  if(buffCount >= buffSize){
      buffCount = 0;
      buffEmpty[whichBuff] = true;
      whichBuff = !whichBuff;

  }

}


void TMRpcm::stopPlayback(){
  playing = 0;
  *TIMSK[tt] &= ~( _BV(ICIE1) | _BV(TOIE1) );
  *OCRnA[tt] = 10;
  *OCRnB[tt] = resolution-10;

}

void TMRpcm::disable(){
  playing = 0;
  *TIMSK[tt] &= ~( _BV(ICIE1) | _BV(TOIE1) );
  *TCCRnB[tt] &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12) );

}


boolean TMRpcm::isPlaying(){
	return playing;
}

