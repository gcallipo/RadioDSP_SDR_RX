/**
  ******************************************************************************
  * @file    RadioDSP_SDR_RX.c
  * @author  Giuseppe Callipo - IK8YFW - ik8yfw@libero.it
  * @version V1.0.5
  * @date    21-02-2021
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
#include "RDSP_general_includes.h"
#include "RDSP_controls.h"
#include "RDSP_display.h"
#include "RDSP_noise_reduction.h"
#include "RDSP_convolutional.h"

//************************************************************************
// Enanched ILI9341 display driver
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

//************************************************************************
// Si5351 vfo driver
Si5351      si5351;

//************************************************************************
// Using pins 4 and 5 on teensy 4.0 for A/B tuning encoder
Encoder Position(4, 5);

//************************************************************************
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
// Audio components for the convolutional blocks
AudioRecordQueue         Q_in_L;
AudioRecordQueue         Q_in_R;
AudioPlayQueue           Q_out_L;
AudioPlayQueue           Q_out_R;

//************************************************************************
// Audio Input block connections
AudioConnection c1(IQinput, 0, preProcessor, 0);
AudioConnection c2(IQinput, 1, preProcessor, 1);

// Spectrum RF analisys
AudioConnection c2f1(IQinput, 0, biquad1, 0);  
AudioConnection c2f2(IQinput, 1, biquad2, 0);  
AudioConnection c2f11(biquad1, 0, FFT, 0);  
AudioConnection c2f22(biquad2, 0, FFT, 1);  

// SDR path 
AudioConnection a3(preProcessor, 0, SDR, 0);
AudioConnection a4(preProcessor, 1, SDR, 1);

// Convolutional path 
AudioConnection c5(SDR, 0, Q_in_L, 0);
AudioConnection c6(SDR, 1, Q_in_R, 0);
AudioConnection c6a(Q_out_L, 0, AudioFFT, 0);  
AudioConnection c7(Q_out_L, 0, audio_out, 0);
AudioConnection c8(Q_out_R, 0, audio_out, 1);

//**************************************************************************
// Timer management
Metro tuner = Metro(50);
Metro commands = Metro(200);
//**************************************************************************

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
 
  //SDR.enableNoiseBlanker();             // You can choose whether this is necessary
  //SDR.setNoiseBlankerThresholdDb(20.0); // This works on my setup
  SDR.disableNoiseBlanker();              // You can choose whether this is necessary
 
  SDR.setInputGain(1.0);                  // You mave have to experiment with these
  SDR.setOutputGain(0.5);
  SDR.setIQgainBalance(1.020);            // This was foumd by experimentation
 
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
  AudioMemory(40);
  AudioNoInterrupts();

  // Filter for DC cleaning before FFT Panadapter
  biquad1.setHighpass(0,500,0.5);
  biquad2.setHighpass(0,500,0.5);
  
  // Place the enable as first operation ...
  codec.enable();
  codec.inputSelect(AUDIO_INPUT_LINEIN);
  codec.volume(1.0);
  codec.lineInLevel(8);  // Set codec input voltage level to most sensitive 8
  codec.lineOutLevel(0); // Set codec output voltage level to most sensitive 10
  
  // Final audio setting
  codec.unmuteHeadphone();
  //codec.unmuteLineout(); //unmute the audio output
  //codec.adcHighPassFilterEnable();
  codec.adcHighPassFilterDisable();

  // Initialize only LMS noise reduction
  Init_LMS_NR (15);

  // Reenable interrupts 
  AudioInterrupts();
  delay(500);
  SDR.setMute(false);
  
  // Initializzation Convolutional struct for future use
  doConvolutionalInitialize();

  // For test only need additional tuning
  reInitializeFilter(300, 4000);
  showPBT();
 
  delay(500);
}

//************************************************************************
//************************************************************************
// 					MAIN SETUP FUNCTION
//************************************************************************
//************************************************************************

void loop()
{
   // Execute convolutional processing block
  doConvolutionalProcessing(nr_level, true, 300.0, 4000.0);
  
  if (commands.check() == 1)
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
