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

#ifndef RDSP_CONTROLS_H_INCLUDED
#define RDSP_CONTROLS_H_INCLUDED

#include "RDSP_general_includes.h"
#include "RDSP_convolutional.h"
#include "RDSP_display.h"

extern Encoder   Position;
extern AudioSDR  SDR;
extern Si5351    si5351;

//************************************************************************
//      Display current menu level
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
    case L4_PBT_LH:
      {
        MENU_SetButtons("PBT L", "PBT H");
        break;
      }
      
  }
}

//************************************************************************
//      Move to the next menu (when rotate the knob clockw.)
//************************************************************************
void MENU_nextMenuLevel()
{
  if ((iMenuLevel >= 1) && (iMenuLevel < 4)){
    iMenuLevel = iMenuLevel +1;
  }
  MENU_displayMenuLevel();
}

//************************************************************************
//      Move to the previous menu (when rotate the knob anticlockw.)
//************************************************************************
void MENU_prevMenuLevel()
{
  if ((iMenuLevel > 1) && (iMenuLevel <= 4)){
    iMenuLevel = iMenuLevel -1;
  }
  MENU_displayMenuLevel();
}

//************************************************************************
//      Change the tuning step 
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
//      Change the filter width
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
//      Change AGC settings
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
//      Change Noise Reduction settings
//************************************************************************
void setNRMode()
{
  if(nrndx==5)
  {
    nrndx=0;
  }
  else
  {
   nrndx= nrndx+1;
  }
  
  if(nrndx==0)
  { 
   SDR.disableALSfilter();
   SDR.enableAGC();  
   newNR= "";
   nr_level = 0;
  }
  
  if(nrndx==1)
  {
   SDR.enableAGC();  
   SDR.enableALSfilter();
   SDR.setALSfilterNotch();
   SDR.setALSfilterAdaptive();
   newNR= "NOTCH";
  }

  if(nrndx==2)
  {
   SDR.disableALSfilter();
   //SDR.disableAGC();  
   newNR= "DNR 1";
   nr_level = 20;
  }

  if(nrndx==3)
  {
   SDR.disableALSfilter();
   //SDR.disableAGC();  
   newNR= "DNR 2";
   nr_level = 30;
  }

 if(nrndx==4)
  {
   SDR.disableALSfilter();
   //SDR.disableAGC();  
   newNR= "DNR 3";
   nr_level = 40;
  }
  if(nrndx==5)
  {
   SDR.disableALSfilter();
   //SDR.disableAGC();  
   newNR= "DNR 4";
   nr_level = 50;
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
//      Change the rx demodulation
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
   SDR.setAudioFilter(audio2100);
   if (vfoFreq > 10000000){
      TuningOffset = SDR.setDemodMode(CW_USBmode); 
    }else{
      TuningOffset = SDR.setDemodMode(CW_LSBmode); 
    }
   newFilter= "2.1 kHz";
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
    newMode="SAM";
    SDR.setAudioFilter(audioAM);
    TuningOffset = SDR.setDemodMode(SAMmode);
    newFilter= "3.9 kHz";
    fndx=4;
  }
/*
   if(mndx==6)
  {
    newMode="WSPR";
    SDR.setAudioFilter(audioWSPR);  // WSPR output is 200Hz wide, centered on 1500 Hz
    TuningOffset = SDR.setDemodMode(USBmode);  // WSPR is always USB
    newFilter= "200 Hz";
    fndx=0; // check it !
    
  }
 */ 
   if(mndx==6)
  {
    newMode="RTTY";
    SDR.setAudioFilter(audio2100);
    TuningOffset = SDR.setDemodMode(USBmode);
    newFilter= "2.1 kHz";
    fndx=1;
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
//         VFO Initializzation routine
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
//      Send actual frequency value to SI5351 VFO
//************************************************************************
void sendFreq()
{ //delay(10);
  si5351.set_freq((vfoFreq - TuningOffset) * 400ULL, SI5351_CLK0); // generating 4 x frequency ... set 400ULL to 100ULL for 1x frequency
}

//************************************************************************
//      Show current frequency on sceen
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
//         Change PBT Filter settings
//************************************************************************
boolean checkPBT_Increase(){

   if (iMenuLevel == L4_PBT_LH){
          // Increase LOCUT
      if (digitalRead(BUTTON_D3) == LOW){
          dFLoCut = (dFLoCut + 50)<=MAX_LOW?(dFLoCut + 50):dFLoCut;
          reInitializeFilter(dFLoCut, dFHiCut);
          showPBT();
          return true;
      }
      // Increase HICUT
      else if (digitalRead(BUTTON_D6) == LOW){
          dFHiCut = (dFHiCut + 50)<=MAX_HI?(dFHiCut + 50):dFHiCut;
          reInitializeFilter(dFLoCut, dFHiCut);
          showPBT();
          return true;
      }
    }
    return false;
}

boolean checkPBT_Decrease(){

   if (iMenuLevel == L4_PBT_LH){
        // Decrease LOCUT
  if (digitalRead(BUTTON_D3) == LOW){
      dFLoCut = (dFLoCut - 50)>MIN_LOW?(dFLoCut - 50):dFLoCut;
      if (dFLoCut<0.0) dFLoCut=0.0;
      reInitializeFilter(dFLoCut, dFHiCut); 
      showPBT();
      return true;
  }

        // Decrease HICUT
  else  if (digitalRead(BUTTON_D6) == LOW){
      dFHiCut = (dFHiCut - 50)>MIN_HI?(dFHiCut - 50):dFHiCut;
      reInitializeFilter(dFLoCut, dFHiCut);
      showPBT();
      return true;
  }
    }

    return false;
}

//************************************************************************
//         Change Frequency method
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
          // if we are changing the PBT
          boolean bChecked = checkPBT_Increase();
          if (!bChecked){
            // change freq
            vfoFreq = (Freq + Fstep);
            if (vfoFreq >= topFreq)
            {
              vfoFreq = topFreq;
            }
          }
        }

        if (newPosition < oldPosition)
        {
          // if we are changing the PBT
          boolean bChecked = checkPBT_Decrease();
          if (!bChecked){
            // change freq
            vfoFreq = (Freq - Fstep);
            if (vfoFreq <= bottomFreq)
            {
              vfoFreq = bottomFreq;
            }
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
//        Check the menu level
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
         case L4_PBT_LH:
          {
            // do nothing
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
        case L4_PBT_LH:
          {
            // do nothing
            break;
          }    
      }
    }
  }
}

#endif /* RDSP_CONTROLS_H_INCLUDED */

/**************************************END OF FILE****/
