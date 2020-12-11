/**
  ******************************************************************************
  * @file    RadioDSP_SDR_RX.c
  * @author  Giuseppe Callipo - IK8YFW - ik8yfw@libero.it
  * @version V1.0.1
  * @date    12-12-2020
  * @brief   Main routine file
  *
  ******************************************************************************
  *
  *                     THE RADIO DSP SDR RX - PROJECT
  *
  * This project RadioDSP SDR RX define an experimental open platform to build
  * a Software Defined Radio based on Teensy 4.0 processing module. 
  *
  *
  * The software was derived by the BackPack Sdr Receiver ideated by Keith Myles 
  * and use the AudioSDR library by Derek Rowel as main SDR engine.
  *
  * NOTE: this is an experimental project and the functions can be changed
  * without any advise.
  * The RadioDSP use some parts of the ARM Cortex Microcontroller Software
  * Interface Standard (CMSIS).
  * The RadioDSP uses examples and projects freeware and available in the opensource
  * community.
  *
  * The RadioDSP openSource software is released under the license:
  *              Common Creative - Attribution 3.0
  ******************************************************************************
*/

//**************************************************************************
// New complex FFT library for full bandwidth spectrum 
// from I & Q --> 44.1kHz
#include "analyze_fft256iq.h" 

// This is the Kurt E. ILI9341 display driver
// Available on : https://github.com/KurtE/ILI9341_t3n
#include "ILI9341_t3n.h"
#include "ILI9341_t3n_font_Arial.h"
#include "ILI9341_t3n_font_ArialBold.h"

#include <SPI.h>
#include <Wire.h>         //protocol to talk to si5351
#include <WireIMXRT.h>    //gets installed with wire.h
#include <WireKinetis.h>  //
#include <Encoder.h>
#include <Metro.h>       // task scheduler

// This use the SI5351 library by Jason Milldrum
// https://github.com/etherkit/Si5351Arduino
#include <si5351.h>

// Reference to the Audio Teensy Library
#include <Audio.h>

// Reference to CMSIS platform
#include "arm_math.h"
#include "arm_const_structs.h"

// This is the reference to the AudioSDR library by Derek Rowel
#include "AudioSDRlib.h"

//************************************************************************
// Define 3 buttons for menu handling
#define BUTTON_D2   2
#define BUTTON_D3   3
#define BUTTON_D6   6

#define MENU_MODE    0
#define RUNNING_MODE 1

#define L1_MODE_TS   1
#define L2_FLT_NR    2
#define L3_SCOPE_AGC 3

//*************************************************************************
#define MAX_DECIMAL_TUNING 2
#define MAX_WATERFALL 50
#define POSITION_SPECTRUM 159

//************************************************************************
// For optimized ILI9341_t3 library
#define TFT_DC    9
#define TFT_CS    10
#define TFT_RST   255  // 255 = unused, connect to 3.3V
#define TFT_MOSI  11
#define TFT_SCLK  13
#define TFT_MISO  12
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

Si5351 si5351;

//************************************************************************
// Using pins 4 and 5 on teensy 4.0 for A/B tuning encoder
Encoder Position(4, 5);

//************************************************************************
// Audio library classes
AudioInputI2S          IQinput;
AudioSDRpreProcessor   preProcessor;
AudioSDR               SDR;
AudioOutputI2S         audio_out;
AudioControlSGTL5000   codec;
AudioAnalyzeFFT256IQ   FFT;
AudioAnalyzeFFT1024     AudioFFT;
AudioFilterBiquad      biquad1;
AudioFilterBiquad      biquad2;

//************************************************************************
// Audio Block connections
AudioConnection c1(IQinput, 0, preProcessor, 0);
AudioConnection c2(IQinput, 1, preProcessor, 1);

AudioConnection c2f1(IQinput, 0, biquad1, 0);  
AudioConnection c2f2(IQinput, 1, biquad2, 0);  

AudioConnection c2f11(biquad1, 0, FFT, 0);  
AudioConnection c2f22(biquad2, 0, FFT, 1);  

AudioConnection c3(preProcessor, 0,  SDR, 0);
AudioConnection c4(preProcessor, 1,  SDR, 1);

AudioConnection c2f4a(SDR, 0, AudioFFT, 0);  

AudioConnection c5(SDR, 0, audio_out, 0);
AudioConnection c6(SDR, 1, audio_out, 1);

//************************************************************************
// Globals
int         		iMode = 1;
int         		iMenuLevel = 1;
long 				newPosition = 0;
long 				oldPosition = 0;
static const long 	topFreq = 30000000;// sets receiver upper frequency limit 30 MHz
static const long 	bottomFreq = 30000;// sets the receiver lower frequency limit 30 kHz
volatile uint32_t 	Freq = 7055000;// previous frequency
volatile uint32_t 	Fstep = 0; // sets the tuning increment
volatile uint32_t 	vfoFreq = 7050000;
uint32_t 			TuningOffset;

String 	mode;
String 	newMode;
String 	Ts;
String 	newTs;
String 	filter;
String 	newFilter;
String 	nr;
String 	newNR;
String 	agc;
String  newAgc;
float   uvold = 0; 

int 	tndx = 3; //100 kHz
int 	mndx = 3; //LSB
int 	fndx = 2; //audio2700 - 2.4 kHz
int 	nrndx = 0; //NR disabled
int 	andx = 2; //Agc medium
int 	lock = 0;
int   nscope = 1; // 0 = Panadapter - 1 = Audioscope

int 	minTS = 1;
int 	maxTS = 6;

uint16_t WaterfallData[MAX_WATERFALL][512] = {1};
uint16_t SpectrumView[512] = {1};
uint16_t SpectrumViewOld[512] = {1};

// Frame buffer DMA memory for ILI9341_t3n display
DMAMEM uint16_t fb1[320 * 240];

//**************************************************************************
// Timer management
Metro tuner = Metro(50);
Metro touch = Metro(200);

// Variable timer
unsigned long SPC_SLOW = 200;
unsigned long timeSpc = 0;
unsigned long lastTimeSpc = 0;
unsigned long intervalSpc = SPC_SLOW;
//**************************************************************************

//**************************************************************************
//**************************************************************************
// METHODS
//**************************************************************************
//**************************************************************************

//************************************************************************
// 				VFO Initializzation routine
//************************************************************************
void initVfo()
{
  delay(50);
  
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(33000, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  //si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  si5351.set_freq(vfoFreq * 400ULL , SI5351_CLK0); //this generates 4 x your input freq
  //si5351.set_freq(vfoFreq *100ULL , SI5351_CLK0); //this generates 1 x your input freq
}

//************************************************************************
// 				Display Initializzation routine
//************************************************************************
void initDisplay()
{
  //*********************************************************
  // spi clock	
  uint32_t spi_clock = 70000000; // default was 30000000;
  uint32_t spi_clock_read = 10000000;
  tft.begin(spi_clock, spi_clock_read); 
  tft.setFrameBuffer(fb1);
  tft.useFrameBuffer(true);
   
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  delay(200);
  
  tft.drawLine(0, 50, 320, 50, ILI9341_CYAN);
  tft.drawLine(260, 50, 260, 220, ILI9341_CYAN);
  tft.drawLine(0, 220, 320, 220, ILI9341_CYAN);
  tft.drawLine(175, 0, 175, 51, ILI9341_CYAN);

  // Start the DMA update ...
  tft.updateScreenAsync(true);

}

//************************************************************************
// 				Change Frequency method
//************************************************************************
void setFreq()
{
  if (iMode == RUNNING_MODE) {
    newPosition = Position.read();
    if (newPosition != oldPosition)
    { 
      // Speedup the scope ...
      intervalSpc = 0;

      if (newPosition > oldPosition  || newPosition < oldPosition )
      {
        if (newPosition > oldPosition)
        {
          vfoFreq = (Freq + Fstep);

          if (vfoFreq >= topFreq)
          {
            vfoFreq = topFreq;
          }
        }

        if (newPosition < oldPosition)
        {
          vfoFreq = (Freq - Fstep);
          if (vfoFreq <= bottomFreq)
          {
            vfoFreq = bottomFreq;
          }
        }
      }
      showFreq(); // show freq on display
      sendFreq(); // send freq to SI5351
      oldPosition = newPosition; //update oldposition
    }else{
      // Slow down the scope ...
      intervalSpc = SPC_SLOW;
    }
  } else {
    newPosition = Position.read();
    if (newPosition != oldPosition)
    {
      if (newPosition > oldPosition + 5 || newPosition < oldPosition - 5)
      {
        if (newPosition > oldPosition)
        {
          MENU_nextMenuLevel();
        }
        if (newPosition < oldPosition)
        {
          MENU_prevMenuLevel();
        }
        oldPosition = newPosition; //update oldposition
      }
    }
  }
}

//************************************************************************
// 				Check the menu level
//************************************************************************
void checkCmd()
{
  if (digitalRead(BUTTON_D2) == LOW) {
   iMode = (iMode == MENU_MODE) ? RUNNING_MODE : MENU_MODE;
   tft.fillRect(99, 222, 150, 240, ILI9341_BLACK );
 
   if (iMode == MENU_MODE) {
     tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
     tft.setFont(Arial_10_Bold);
     tft.setCursor(100, 226);
     tft.print("MENU SET");
   }
  }

  if (iMode == RUNNING_MODE) {

    if (digitalRead(BUTTON_D3) == LOW) {
      switch (iMenuLevel) {
        case L1_MODE_TS:
          {
            tuningMode();
            break;
          }
        case L2_FLT_NR:
          {
            filterMode();
            break;
          }
        case L3_SCOPE_AGC:
          {
            setScopeMode();
            break;
          }
      }
    } else if (digitalRead(BUTTON_D6) == LOW) {
      switch (iMenuLevel) {
        case L1_MODE_TS:
          {
            tuningStep(0);
            break;
          }
        case L2_FLT_NR:
          {
            setNRMode();
            break;
          }
        case L3_SCOPE_AGC:
          {
            setAgc();
            break;
          }
      }
    }
  }
}

//************************************************************************
// 			Move to the next menu (when rotate the knob clockw.)
//************************************************************************
void MENU_nextMenuLevel()
{
  if ((iMenuLevel >= 1) && (iMenuLevel < 4)){
    iMenuLevel = iMenuLevel +1;
  }
  MENU_displayMenuLevel();
}

//************************************************************************
// 			Move to the previous menu (when rotate the knob anticlockw.)
//************************************************************************
void MENU_prevMenuLevel()
{
  if ((iMenuLevel > 1) && (iMenuLevel <= 4)){
    iMenuLevel = iMenuLevel -1;
  }
  MENU_displayMenuLevel();
}

//************************************************************************
// 			Display current menu level
//************************************************************************
void MENU_displayMenuLevel() {
  
  switch (iMenuLevel) {
    case L1_MODE_TS:
      { 
        MENU_SetButtons("MODE", "TS");
        break;
      }
    case L2_FLT_NR:
      {
        MENU_SetButtons("FLT", "NR");
        break;
      }
    case L3_SCOPE_AGC:
      {
        MENU_SetButtons("SCOPE", "AGC");
        break;
      }
  }
}
//************************************************************************
// 			Place button display on screen
//************************************************************************
void  MENU_SetButtons(String leftButtonLabel, String rightButtonLabel){
  
      tft.fillRect(1, 222, 60, 240, ILI9341_BLACK );
      tft.fillRect(220, 222, 300, 240, ILI9341_BLACK );
      tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
      tft.setFont(Arial_10_Bold);
      tft.setCursor(5, 226);
      tft.print(leftButtonLabel);
      tft.setCursor(270, 226);
      tft.print(rightButtonLabel);
};

//************************************************************************
// 			Change the tuning step 
//************************************************************************
void tuningStep(int iMaxTuning)
{
  if (iMaxTuning>0){
    tndx = iMaxTuning;
    maxTS = iMaxTuning;
  }
  
  if (tndx == 0)
  {
    Fstep = 1;
    newTs = "1Hz";
    tft.fillRect(202, 37, 320, 10,   ILI9341_BLACK );
    tft.fillRect(204, 37, 15, 4,    ILI9341_GREEN );
  }
  if (tndx == 1)
  {
    Fstep = 10;
    newTs = "10Hz";
    tft.fillRect(202, 37, 320, 10,    ILI9341_BLACK );
    tft.fillRect(288, 37, 15, 5,    ILI9341_GREEN );
  }
  if (tndx == 2)
  {
    Fstep = 100;
    newTs = "100Hz";
    tft.fillRect(200, 37, 320, 10,    ILI9341_BLACK );
    tft.fillRect(272, 37, 15, 5,    ILI9341_GREEN );
  }
  if (tndx == 3)
  {
    Fstep = 1000;
    newTs = "1kHz";
    tft.fillRect(200, 37, 320, 10,    ILI9341_BLACK );
    tft.fillRect(248, 37, 15, 5,    ILI9341_GREEN );
  }
  if (tndx == 4)
  {
    Fstep = 10000;
    newTs = "10 kHz";
    tft.fillRect(200, 37, 320, 10,    ILI9341_BLACK );
    tft.fillRect(232, 37, 15, 5,    ILI9341_GREEN );
  }
  if (tndx == 5)
  {
    Fstep = 100000;
    newTs = "100 kHz";
    tft.fillRect(200, 37, 320, 10,    ILI9341_BLACK );
    tft.fillRect(216, 37, 15, 5,    ILI9341_GREEN );
  }
  if (tndx == 6)
  {
    Fstep = 1000000;
    newTs = "1 MHz";
    tft.fillRect(200, 37, 320, 10,    ILI9341_BLACK );
    tft.fillRect(200, 37, 15, 5,    ILI9341_GREEN );
  }
  if (tndx == maxTS)
  {
    tndx = minTS; // minimum step set to 10 Hz
  }
  else
  {
    tndx = tndx + 1;
  }
  delay(200);
}

//************************************************************************
// 			Change the filter width
//************************************************************************
void filterMode()
{
  if(fndx==0)
  { 
   SDR.setAudioFilter(audioCW);
   newFilter= "500 Hz";
  }
 
  if(fndx==1)
  {
   SDR.setAudioFilter(audio2100);
   newFilter= "2.1 kHz";
  }

  if(fndx==2)
  {
   SDR.setAudioFilter(audio2700);
   newFilter= "2.7 kHz";
  }

  if(fndx==3)
  {
    SDR.setAudioFilter(audio3100);
    newFilter= "3.1 kHz";
  }
  
  if(fndx==4)
  {
    SDR.setAudioFilter(audioAM);
    newFilter= "3.9 kHz";
  }

  if(fndx==4)
  {
    fndx=0;
  }
  else
  {
   fndx= fndx+1;
  }
  showFilter();
  delay(200);
}

//************************************************************************
// 			Change AGC settings
//************************************************************************
void setAgc()
{
  if(andx==0)
  { 
   SDR.setAGCmode(AGCoff);
   newAgc= "AGC O";
  }
 
  if(andx==1)
  {
   SDR.setAGCmode(AGCfast);
   newAgc= "AGC F";
  }

  if(andx==2)
  {
   SDR.setAGCmode(AGCmedium);
   newAgc= "AGC M";
  }

  if(andx==3)
  {
    SDR.setAGCmode(AGCslow);
   newAgc= "AGC S";
  }
  
  if(andx==3)
  {
    andx=0;
  }
  else
  {
   andx= andx+1;
  }
  showAgcMode();
  delay(200);
}

//************************************************************************
// 			Change Noise Reduction settings
//************************************************************************
void setNRMode()
{
  if(nrndx==0)
  { 
   SDR.disableALSfilter();
   newNR= "";
  }
  if(nrndx==1)
  {
   SDR.enableALSfilter();
   SDR.setALSfilterPeak();
   newNR= "NR PK";
  }

  if(nrndx==2)
  {
   SDR.enableALSfilter();
   SDR.setALSfilterNotch();
   newNR= "NR NT";
  }

  if(nrndx==2)
  {
    nrndx=0;
  }
  else
  {
   nrndx= nrndx+1;
  }
  showNRMode();
  delay(200);
}

//************************************************************************
//       Change Scope Mode
//************************************************************************
void setScopeMode()
{
  if(nscope==0)
  { 
   newNR= "Panad";
  }
  if(nscope==1)
  {
   newNR= "Audio";
  }
  if(nscope==1)
  {
    nscope=0;
  }
  else
  {
   nscope= nscope+1;
  }
  // Clean scope zone
  tft.fillRect(0, 52, 260, 160, ILI9341_BLACK );

  //showScopeMode();
  delay(200);
}

//************************************************************************
// 			Change the rx demodulation
//************************************************************************
void tuningMode()
{
   if(mndx==0)
  {
    newMode="CW N";
    SDR.setAudioFilter(audioCW);
    if (vfoFreq > 10000000){
      TuningOffset = SDR.setDemodMode(CW_USBmode); 
    }else{
      TuningOffset = SDR.setDemodMode(CW_LSBmode); 
    }
    newFilter= "500 Hz";
  }
 
  if(mndx==1)
  {
   newMode="CW";
   SDR.setAudioFilter(audio2700);
   if (vfoFreq > 10000000){
      TuningOffset = SDR.setDemodMode(CW_USBmode); 
    }else{
      TuningOffset = SDR.setDemodMode(CW_LSBmode); 
    }
   newFilter= "2.7 kHz";
   fndx=2;
  }

  if(mndx==2)
  {
   newMode="USB";
   SDR.setAudioFilter(audio2700);
   TuningOffset = SDR.setDemodMode(USBmode); 
   newFilter= "2.7 kHz";
   fndx=2;
  }

  if(mndx==3)
  {
    newMode="LSB";
    SDR.setAudioFilter(audio2700);
    TuningOffset = SDR.setDemodMode(LSBmode); 
    newFilter= "2.7 kHz";
    fndx=2;
  }
  
  if(mndx==4)
  {
    newMode="AM";
    SDR.setAudioFilter(audioAM);
    TuningOffset = SDR.setDemodMode(AMmode);
    newFilter= "3.9 kHz";
    fndx=4;
  }
  
   if(mndx==5)
  {
    newMode="RTTY";
    SDR.setAudioFilter(audio2100);
    TuningOffset = SDR.setDemodMode(USBmode);
    newFilter= "2.1 kHz";
    fndx=1;
  }

   if(mndx==6)
  {
    newMode="WSPR";
    SDR.setAudioFilter(audioWSPR);  // WSPR output is 200Hz wide, centered on 1500 Hz
    TuningOffset = SDR.setDemodMode(USBmode);  // WSPR is always USB
    newFilter= "200 Hz";
    fndx=0; // check it !
    
  }

  if(mndx==6)
  {
    mndx=0;
  }
  else
  {
   mndx= mndx+1;
  }
  showMode();
  showFilter();
  delay(200);
}

//************************************************************************
// 			Display Mode on screen
//************************************************************************
void showMode()
{
  if (mode != newMode)
  {
    tft.setFont(Arial_10_Bold);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(270, 60);
    tft.print(mode);
    tft.setCursor(270, 60 );
    tft.setTextColor(ILI9341_WHITE);
    tft.print(newMode);
    mode = newMode;
  }
}

//************************************************************************
// 			Display Filter settings on screen
//************************************************************************
void showFilter()
{
  if (filter != newFilter)
  {
    tft.setFont(Arial_10_Bold);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(270, 75);
    tft.print(filter);
    tft.setCursor(270, 75 );
    tft.setTextColor(ILI9341_WHITE);
    tft.print(newFilter);
    filter = newFilter;
  }
}

//************************************************************************
// 			Display NR settings on screen
//************************************************************************
void showNRMode()
{
  if (nr != newNR)
  {
    tft.setFont(Arial_10_Bold);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(270, 105);
    tft.print(nr);
    tft.setCursor(270, 105 );
    tft.setTextColor(ILI9341_WHITE);
    tft.print(newNR);
    nr = newNR;
  }
}

//************************************************************************
// 			Display AGC settings on screen
//************************************************************************
void showAgcMode()
{
  if (agc != newAgc)
  {
    tft.setFont(Arial_10_Bold);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(270, 90);
    tft.print(agc);
    tft.setCursor(270, 90 );
    tft.setTextColor(ILI9341_WHITE);
    tft.print(newAgc);
    agc = newAgc;
  }
}

//************************************************************************
// 			Display Tuning settings on screen
//************************************************************************
void showTS()
{
  if (Ts != newTs)
  {
    tft.setFont(Arial_8);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(5, 40);
    tft.print(Ts);
    tft.setTextColor(ILI9341_WHITE  );
    tft.setCursor(5, 40);
    tft.print(newTs);
    Ts = newTs;
  }
}

//************************************************************************
// 			Send actual frequency value to SI5351 VFO
//************************************************************************
void sendFreq()
{ delay(50);
  si5351.set_freq((vfoFreq - TuningOffset) * 400ULL, SI5351_CLK0); // generating 4 x frequency ... set 400ULL to 100ULL for 1x frequency
}

//************************************************************************
// 			Show current frequency on sceen
//************************************************************************
void showFreq()
{
  if (Freq != vfoFreq)
  {

     // Check for automatic step down TS
    if (vfoFreq < 1999999 && vfoFreq >= 1000000)
    {
      if (Fstep == 1000000)
      {
       tuningStep(5);
      }
    }

    // Check for automatic step down TS
    if (vfoFreq < 199999 && vfoFreq >= 100000)
    {
      if (Fstep == 100000)
      {
       tuningStep(4);
      }
    }

    // Check for automatic step down TS
    if (vfoFreq < 19999 && vfoFreq >= 10000)
    {
      if (Fstep == 10000)
      {
       tuningStep(3);
      }
    }

    if (vfoFreq < 99999 )
    {
      tft.setFont(Arial_20);
      tft.setCursor(184, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(200, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(216, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(232, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(232, 15);
      tft.setTextColor(ILI9341_WHITE);
      tft.print((float)vfoFreq / 1000, MAX_DECIMAL_TUNING);

      // Set the maximum allowabe tuning step range
      maxTS = 4;
    }
    
     if (vfoFreq < 999999 && vfoFreq >= 99999)
    {
      tft.setFont(Arial_20);
      tft.setCursor(184, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(200, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(216, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(216, 15);
      tft.setTextColor(ILI9341_WHITE);
      tft.print((float)vfoFreq / 1000, MAX_DECIMAL_TUNING);

      // Set the maximum allowabe tuning step range
      maxTS = 5;
    }
   
     if (vfoFreq < 9999999 && vfoFreq >= 999999)
    {
      tft.setFont(Arial_20);
      tft.setCursor(184, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(200, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(200, 15);
      tft.setTextColor(ILI9341_WHITE);
      tft.print((float)vfoFreq / 1000, MAX_DECIMAL_TUNING);

      // Set the maximum allowabe tuning step range
      maxTS = 6;
    }
    
    if (vfoFreq >= 9999999)
    
    {
      tft.setCursor(200, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setFont(Arial_20);
      tft.setCursor(184, 15);
      tft.setTextColor(ILI9341_BLACK);
      tft.print((float)Freq / 1000, MAX_DECIMAL_TUNING);
      tft.setCursor(184, 15);
      tft.setTextColor(ILI9341_WHITE);
      tft.print((float)vfoFreq / 1000, MAX_DECIMAL_TUNING);

      // Set the maximum allowabe tuning step range
      maxTS = 6;
    }
  }
  Freq = vfoFreq; // update oldfreq
}

//************************************************************************

//************************************************************************
//       Show Helf Panadapter 22KHz & Audio Spectrum - AF-FFT
//************************************************************************
void Update_DoubleSpectrum()
{
  // Display Half panadapter
  Update_Panadapter(1);

  // Display the Audio FFT
  Update_AudioSpectrum();

  tft.drawLine(0, 70, 260, 70, ILI9341_CYAN);
  tft.drawLine(140, 50, 140, 220, ILI9341_CYAN);

  tft.setFont(Arial_9_Bold);
  tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
  tft.setCursor(20, 55);
  tft.print("RX-SCOPE");
  tft.setCursor(160, 55);
  tft.print("AF-FFT");
      
  //tft.drawLine(0, 220, 320, 220, ILI9341_CYAN);
  //tft.drawLine(175, 0, 175, 51, ILI9341_CYAN);

}

//************************************************************************
//       Show single Audio Spectrum - AF-FFT
//************************************************************************
void Update_AudioSpectrum()
{
  int bar = 0;
  int xPos = 0;
  int low = 5;

  // Spectrum
  for (int x = 0; x <= 100; x++)
  {
    bar = abs(AudioFFT.output[x]*5);
    if (bar > 70) bar = 70;
    tft.drawFastVLine(146 + (xPos), (POSITION_SPECTRUM -1) - bar, bar, ILI9341_ORANGE); //draw green bar
    tft.drawFastVLine(146 + (xPos), (POSITION_SPECTRUM -100), 100 - bar, ILI9341_BLACK);  //finish off with black to the top of the screen
    xPos++;
  }

  tft.setFont(Arial_8);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(146, POSITION_SPECTRUM + 5);
  tft.print("0  0.5k  1.5k  2.5k  3.5k");
}

//************************************************************************
// 			Show full 44KHz wide signal spectrum & Wwaterfall
//************************************************************************
void Update_Panadapter(int iDisplayMode)
{
  int bar = 0;
  int xPos = 0;
  int low = 0;
  int scale=5;
  float avg = 0.0;
  float LPFcoeff = 0.7; 
  int iMaxCols = 127; 

  // Display size
  if (iDisplayMode == 0){
    // Full panadapter
    iMaxCols = 127;
    tft.drawLine(0, 70, 260, 70, ILI9341_CYAN);
    tft.setFont(Arial_9_Bold);
    tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
    tft.setCursor(20, 55);
    tft.print("RX-SCOPE");
  }else{
    // Half panadapter
    iMaxCols = 64;
  }

  // Pre process spectrum data ...
  for (int x = 0; x < 256; x++){
   // Frequency Smoothing 
   // Moving window - weighted average of 5 points of the spectrum to smooth spectrum in the frequency domain
   // Weights:  x: 50% , x-1/x+1: 36%, x+2/x-2: 14% 
   if ((x > 1) && (x < 254))
   {       
       avg = FFT.output[x]*0.7 + FFT.output[x-1]*0.3 + 
                  FFT.output[x-2]*0.15 + FFT.output[x+1]*0.3 + 
                  FFT.output[x+2]*0.15;         
   }else{
       avg =  FFT.output[x];
   }

   // Time Smoothing
   // low pass filtering of the spectrum pixels to smooth/slow down spectrum in the time domain
   // experimental LPF for spectrum:  
   SpectrumView[x] = LPFcoeff * 2 * sqrt (abs(avg)*scale) + (1 - LPFcoeff) * SpectrumViewOld[x];
  
   // Update the value for the next step ...
   SpectrumViewOld[x]= SpectrumView[x];
  }
    // Spectrum
    for (int x = 0; x <= iMaxCols; x++)
    {
      WaterfallData[0][xPos] = abs(SpectrumView[x*2]);
      bar = WaterfallData[0][xPos];
      if (bar > 80)
        bar = 80;
      tft.drawFastVLine(2 + (xPos*2), (POSITION_SPECTRUM -1) - bar, bar, ILI9341_GREEN); //draw green bar
      tft.drawFastVLine(2 + (xPos*2), (POSITION_SPECTRUM -100), 100 - bar, ILI9341_BLACK);  //finish off with black to the top of the screen
      xPos++;
    }
  
    // Waterfall
    for (int row = MAX_WATERFALL-1; row >= 0; row--)
      for (int col = 0; col <= iMaxCols; col++)
      {
        WaterfallData[row][col] = WaterfallData[row - 1][col];
  
        if (WaterfallData[row][col] >= low + 75)
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_RED);
  
        else if ((WaterfallData[row][col] >= low + 60) && (WaterfallData[row][col] < low + 75))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_MAGENTA);
          
        else if ((WaterfallData[row][col] >= low + 45) && (WaterfallData[row][col] < low + 60))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_ORANGE);  
  
        else if ((WaterfallData[row][col] >= low + 30) && (WaterfallData[row][col] < low + 45))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_YELLOW);
  
        else if ((WaterfallData[row][col] >= low + 20) && (WaterfallData[row][col] < low + 30))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_BLUE);
  
        else if (WaterfallData[row][col] < low + 20)
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_BLACK);
     }
  
      // Display the carrier Tunig line
      tft.drawFastVLine(72, 70, 90, ILI9341_RED); //draw green bar
      tft.drawFastVLine(100, 70, 90, ILI9341_RED); //draw green bar
}

//************************************************************************
// 			Evaluate value of Smeter taking some vales of rc signal bins
//************************************************************************
void Update_smeter(){
  
  float specVal = 0.0; 
  
  // Evaluate the medium signal data for the carrier
  for(int m = 75; m<=85; m++)  { specVal = specVal+FFT.output[m]; }
  displayPeak(abs(specVal/5));

}

void displayPeak(float s_sample) {
  
   float uv, dbuv, s;// microvolts, db-microvolts, s-units
   char string[80];   // print format stuff

   uv= s_sample/10 ;//* 1000;
   
   // do some filter to smoth the change
   uv = 0.1*uv+0.9*uvold;
   uvold = uv;

   dbuv = 20.0*log10(uv);
   tft.fillRect(5, 10, 160,40,    ILI9341_BLACK );
   if (dbuv >0){
    tft.fillRect(5, 15,abs(dbuv*2),15,    ILI9341_GREEN );
   }
    
   s = 1.0 + (10.0 + dbuv*1.2)/6.0;
   if (s <0.0) s=0.0;
   if (s>9.0)
   {
      dbuv = dbuv - 34.0;
      s = 9.0;
   }
   else dbuv = 0;
     
   // tft.fillRect(120, 5, 130,20,ILI9341_BLACK);
   tft.setFont(Arial_12_Bold);
   tft.setCursor(110, 15);
   tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
     
   if (dbuv == 0) sprintf(string,"S:%1.0f",s);
   else sprintf(string,"S:9+%02.0f",dbuv);
   tft.print(string);
 
}

//************************************************************************
//************************************************************************
// 					MAIN SETUP FUNCTION
//************************************************************************
//************************************************************************
void setup()
{
  
  initDisplay();
  initVfo();
  showFreq();
  tuningStep(0);
  tuningMode();
  MENU_displayMenuLevel();
  
  pinMode(BUTTON_D2, INPUT_PULLUP);
  pinMode(BUTTON_D3, INPUT_PULLUP);
  pinMode(BUTTON_D6, INPUT_PULLUP);
  delay (500);

  preProcessor.startAutoI2SerrorDetection();    // IQ error compensation
  //preProcessor.swapIQ(false);

  SDR.enableAGC();                
  SDR.setAGCmode(AGCmedium);
  newAgc= "AGC M";
  showAgcMode();
  
  SDR.disableALSfilter();
  newNR= "";
  showNRMode();
 
  SDR.enableNoiseBlanker();             // You can choose whether this is necessary
  SDR.setNoiseBlankerThresholdDb(10.0); // This works on my setup
 
  SDR.setInputGain(1.0);                // You mave have to experiment with these
  SDR.setOutputGain(0.5);
  SDR.setIQgainBalance(1.020);          // This was foumd by experimentation
 
  SDR.enableAudioFilter();
  SDR.setAudioFilter(audio2700);
  TuningOffset = SDR.setDemodMode(LSBmode);  
  newFilter= "2.7 kHz";
  fndx=2;
  showFilter();

  FFT.windowFunction(AudioWindowHanning256);
  FFT.averageTogether(30);
  
  AudioFFT.windowFunction(AudioWindowHanning1024);
  AudioFFT.averageTogether(30);

  // Set up the Audio board
  AudioMemory(20);
  AudioNoInterrupts();

  // Filter for DC cleaning before FFT Panadapter
  biquad1.setHighpass(0,300,0.4);
  biquad2.setHighpass(0,300,0.4);
  
  // Place the enable as first operation ...
  codec.enable();
  codec.adcHighPassFilterDisable();
  //codec.adcHighPassFilterEnable();
  
  codec.inputSelect(AUDIO_INPUT_LINEIN);
  codec.volume(0.8);
  codec.lineInLevel(10);  // Set codec input voltage level to most sensitive
  codec.lineOutLevel(13); // Set codec output voltage level to most sensitive
  AudioInterrupts();
  delay(500);
  SDR.setMute(false);

}

//************************************************************************
//************************************************************************
// 					MAIN SETUP FUNCTION
//************************************************************************
//************************************************************************
void loop()
{
  if (touch.check() == 1)
  {
    checkCmd();
  }
  if (tuner.check() == 1)
  {
    setFreq();
  }
 
  if (iMode != MENU_MODE) {
    timeSpc = millis();
    if ((timeSpc - lastTimeSpc) > intervalSpc){
      if (FFT.available()) { 
        
        // Is the Full Panadapter active ?
        if (nscope ==0) Update_Panadapter(0);

        // Update meter
        Update_smeter();
     } 

     // Is the AudioScope Active
     if (AudioFFT.available() && nscope ==1) {
      
        //Update_AudioSpectrum();
        Update_DoubleSpectrum();
     }
      lastTimeSpc = timeSpc;
    }
  }
}

//************************************************************************
//************************************************************************
