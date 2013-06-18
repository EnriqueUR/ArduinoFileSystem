/*Library by TMRh20 2012-2013*/

#ifndef TMRpcm_h   // if x.h hasn't been included yet...
#define TMRpcm_h   //   #define this so the compiler knows it has been included

class TMRpcm
{
 public:
	void stopPlayback();
	void volume(int vol);
	void disable();
	void pause();
	int speakerPin;
	boolean wavInfo(SDFile xFile);
	void play(SDFile sFile);
	boolean pwmMode;
	boolean isPlaying();
	boolean rfPlaying;
	unsigned long SAMPLE_RATE;


 private:
	int lastSpeakPin;
	void setPin();
};




#endif