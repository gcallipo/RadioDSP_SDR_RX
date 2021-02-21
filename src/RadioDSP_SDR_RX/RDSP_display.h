
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
#ifndef RDSP_DISPLAY_H_INCLUDED
#define RDSP_DISPLAY_H_INCLUDED

#include "RDSP_general_includes.h"

// For optimized ILI9341_t3 library
#define TFT_DC    9
#define TFT_CS    10
#define TFT_RST   255  // 255 = unused, connect to 3.3V
#define TFT_MOSI  11
#define TFT_SCLK  13
#define TFT_MISO  12

extern ILI9341_t3n tft;
extern AudioAnalyzeFFT256IQ   FFT;
extern AudioAnalyzeFFT1024     AudioFFT;

uint16_t WaterfallData[MAX_WATERFALL][512] = {1};
uint16_t SpectrumView[512] = {1};
uint16_t SpectrumViewOld[512] = {1};

// Frame buffer DMA memory for ILI9341_t3n display
DMAMEM uint16_t fb1[320 * 240];

//*************************************************************************
// Variable timer
unsigned long       SPC_SLOW = 200;
unsigned long       timeSpc = 0;
unsigned long       lastTimeSpc = 0;
unsigned long       intervalSpc = SPC_SLOW;

//************************************************************************
//         Display Initializzation routine
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
//      Display Mode on screen
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
//      Display Filter settings on screen
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
//      Display NR settings on screen
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
//      Display NR settings on screen
//************************************************************************
void showPBT()
{
    tft.setFont(Arial_10_Bold);
    tft.setCursor(270, 120 );
    tft.setTextColor(ILI9341_WHITE);
    tft.print("PBT");

    tft.setFont(Arial_10_Bold);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(270, 135);
    tft.print((int)oldFLoCut);
    tft.setCursor(270, 135 );
    tft.setTextColor(ILI9341_WHITE);
    tft.print((int)dFLoCut);
    oldFLoCut = dFLoCut;

    tft.setFont(Arial_10_Bold);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(270, 150);
    tft.print((int)oldFHiCut);
    tft.setCursor(270, 150);
    tft.setTextColor(ILI9341_WHITE);
    tft.print((int)dFHiCut);
    oldFHiCut = dFHiCut;
}



//************************************************************************
//      Display AGC settings on screen
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
//      Display Tuning settings on screen
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
//      Place button display on screen
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
//       Show full 44KHz wide signal spectrum & Wwaterfall
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
  
        else if ((WaterfallData[row][col] >= low + 50) && (WaterfallData[row][col] < low + 75))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_MAGENTA);
          
        else if ((WaterfallData[row][col] >= low + 40) && (WaterfallData[row][col] < low + 50))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_ORANGE);  
  
        else if ((WaterfallData[row][col] >= low + 25) && (WaterfallData[row][col] < low + 40))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_YELLOW);
  
        else if ((WaterfallData[row][col] >= low + 15) && (WaterfallData[row][col] < low + 25))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_BLUE);

        else if ((WaterfallData[row][col] >= low + 5) && (WaterfallData[row][col] < low + 15))
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_NAVY);
          
        else if (WaterfallData[row][col] < low + 5)
          tft.drawPixel(2 + (col * 2), POSITION_SPECTRUM + row, ILI9341_BLACK);
     }
  
      // Display the carrier Tunig line
      tft.drawFastVLine(72, 70, 90, ILI9341_RED); //draw green bar
      tft.drawFastVLine(100, 70, 90, ILI9341_RED); //draw green bar
}

//************************************************************************
//      Evaluate value of Smeter taking some vales of rc signal bins
//************************************************************************
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

void Update_smeter(){
  
  float specVal = 0.0; 
  
  // Evaluate the medium signal data for the carrier
  for(int m = 75; m<=85; m++)  { specVal = specVal+FFT.output[m]; }
  displayPeak(abs(specVal/5));

}


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


#endif /* RDSP_DISPLAY_H_INCLUDED */

/**************************************END OF FILE****/
