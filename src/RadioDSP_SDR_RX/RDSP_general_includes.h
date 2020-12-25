/**
  ******************************************************************************
  * @file    
  * @author  Giuseppe Callipo - IK8YFW - ik8yfw@libero.it
  * @version V1.0.2
  * @date    18-12-2020
  * @brief    file
  *
  ******************************************************************************
  *
   */
#ifndef RDSP_GENERAL_INCLUDES_H_INCLUDED
#define RDSP_GENERAL_INCLUDES_H_INCLUDED

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
#define BUTTON_D3   6
#define BUTTON_D6   3

#define MENU_MODE    0
#define RUNNING_MODE 1

#define L1_MODE_TS   1
#define L2_FLT_NR    2
#define L3_SCOPE_AGC 3
//************************************************************************

//************************************************************************
// Globals
int                 iMode = 1;
int                 iMenuLevel = 1;
long                newPosition = 0;
long                oldPosition = 0;
static const long   topFreq = 30000000;// sets receiver upper frequency limit 30 MHz
static const long   bottomFreq = 30000;// sets the receiver lower frequency limit 30 kHz
volatile uint32_t   Freq = 7055000;// previous frequency
volatile uint32_t   Fstep = 0; // sets the tuning increment
volatile uint32_t   vfoFreq = 7050000;
uint32_t            TuningOffset;

//*************************************************************************
// DISPLAY CONTROLS

String              mode;
String              newMode;
String              Ts;
String              newTs;
String              filter;
String              newFilter;
String              nr;
String              newNR;
String              agc;
String              newAgc;
float               uvold = 0; 

int                 tndx = 3; //100 kHz
int                 mndx = 3; //LSB
int                 fndx = 2; //audio2700 - 2.4 kHz
int                 nrndx = 0; //NR disabled
int                 andx = 2; //Agc medium
int                 lock = 0;
int                 nscope = 1; // 0 = Panadapter - 1 = Audioscope

int                 minTS = 1;
int                 maxTS = 6;

//*************************************************************************
#define MAX_DECIMAL_TUNING 2
#define MAX_WATERFALL 50
#define POSITION_SPECTRUM 159


#endif /* RDSP_GENERAL_INCLUDES_H_INCLUDED */

/**************************************END OF FILE****/
