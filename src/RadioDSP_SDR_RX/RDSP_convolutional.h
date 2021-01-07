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

#ifndef RDSP_CONVOLUTIONAL_H_INCLUDED
#define RDSP_CONVOLUTIONAL_H_INCLUDED

#include "RDSP_general_includes.h"
#include "RDSP_convolutional.h"
#include "RDSP_noise_reduction.h"

//************************************************************************
//************************************************************************
extern AudioRecordQueue         Q_in_L;
extern AudioRecordQueue         Q_in_R;
extern AudioPlayQueue           Q_out_L;
extern AudioPlayQueue           Q_out_R;

//************************************************************************
//**************  CONVOLUTIONAL SECTION **********************************
//************************************************************************

/*********************************************************************************************
 *      CONVOLUTIONAL PART - COMMON PART AND DNR PART
 */
#define        BUFFER_SIZE 128
double         SAMPLE_RATE = (double)AUDIO_SAMPLE_RATE_EXACT;  
const uint32_t FFT_L = 256; // 512 1024 2048 4096
uint32_t       FFT_length = FFT_L;
const uint32_t N_B = FFT_L / 2 / BUFFER_SIZE;
uint32_t       N_BLOCKS = N_B;
int16_t        *sp_L;
int16_t        *sp_R;
uint8_t        first_block = 1; 

// complex FFT with the new library CMSIS V4.5
const static   arm_cfft_instance_f32 *S;

// complex iFFT with the new library CMSIS V4.5
const static   arm_cfft_instance_f32 *iS;

float32_t      iFFT_buffer [FFT_L * 2] __attribute__ ((aligned (4)));  // 4096 * 4 = 16kb
float32_t      float_buffer_L [BUFFER_SIZE * N_B];  // 2048 * 4 = 8kb
float32_t      float_buffer_R [BUFFER_SIZE * N_B]; // 2048 * 4 = 8kb

float32_t      FFT_buffer [FFT_L * 2] __attribute__ ((aligned (4)));  // 4096 * 4 = 16kb
float32_t      last_sample_buffer_L [BUFFER_SIZE * N_B];  // 2048 * 4 = 8kb
float32_t      last_sample_buffer_R [BUFFER_SIZE * N_B]; // 2048 * 4 = 8kb

/*********************************************************************************************
 *      FILTER PART - ENABLE ONLY IF CONVOLUTIONAL FILTERING IS ENABLED
 */
#define        FOURPI  (2.0 * TWO_PI)
#define        SIXPI   (3.0 * TWO_PI)
int32_t        sum;
int            idx_t = 0;
float32_t      mean;
uint8_t        FIR_filter_window = 1;
double         FLoCut = 300.0;
double         FHiCut = 4000.0;
double         FIR_Coef_I[(FFT_L / 2) + 1]; // 2048 * 4 = 8kb
double         FIR_Coef_Q[(FFT_L / 2) + 1]; // 2048 * 4 = 8kb
#define        MAX_NUMCOEF (FFT_L / 2) + 1
uint32_t       m_NumTaps = (FFT_L / 2) + 1;

// FFT instance for direct calculation of the filter mask
// from the impulse response of the FIR - the coefficients
const static   arm_cfft_instance_f32 *maskS;
float32_t      FIR_filter_mask [FFT_L * 2] __attribute__ ((aligned (4)));  // 4096 * 4 = 16kb

// hold the actual nr setting
int oldNRLevel = 15;

//************************************************************************
//************************************************************************
//*******************   CONVOLUTIONAL SECTION  ***************************
//************************************************************************

void init_filter_mask()
{
  /****************************************************************************************
     Calculate the FFT of the FIR filter coefficients once to produce the FIR filter mask
  ****************************************************************************************/
  // the FIR has exactly m_NumTaps and a maximum of (FFT_length / 2) + 1 taps = coefficients, 
  // so we have to add (FFT_length / 2) -1 zeros before the FFT
  // in order to produce a FFT_length point input buffer for the FFT
  // copy coefficients into real values of first part of buffer, rest is zero
  for (unsigned i = 0; i < m_NumTaps; i++)
  {
    FIR_filter_mask[i * 2] = FIR_Coef_I [i];
    FIR_filter_mask[i * 2 + 1] = FIR_Coef_Q [i];
  }

  for (unsigned i = FFT_length + 1; i < FFT_length * 2; i++)
  {
    FIR_filter_mask[i] = 0.0;
  }
  // FFT of FIR_filter_mask
  // perform FFT (in-place), needs only to be done once (or every time the filter coeffs change)
  arm_cfft_f32(maskS, FIR_filter_mask, 0, 1);

} // end init_filter_mask



//////////////////////////////////////////////////////////////////////
//  Call to setup complex filter parameters [can process two channels at the same time!]
//  The two channels could be stereo audio or I and Q channel in a Software Defined Radio
// SampleRate in Hz
// FLowcut is low cutoff frequency of filter in Hz
// FHicut is high cutoff frequency of filter in Hz
// Offset is the CW tone offset frequency
// cutoff frequencies range from -SampleRate/2 to +SampleRate/2
//  HiCut must be greater than LowCut
//    example to make 2700Hz USB filter:
//  SetupParameters( 100, 2800, 0, 48000);
//////////////////////////////////////////////////////////////////////

void calc_cplx_FIR_coeffs (double * coeffs_I, double * coeffs_Q, int numCoeffs, double FLoCut, double FHiCut, double SampleRate)
// pointer to coefficients variable, no. of coefficients to calculate, frequency where it happens, stopband attenuation in dB,
// filter type, half-filter bandwidth (only for bandpass and notch)
{
  //calculate some normalized filter parameters
  double nFL = FLoCut / SampleRate;
  double nFH = FHiCut / SampleRate;
  double nFc = (nFH - nFL) / 2.0; //prototype LP filter cutoff
  double nFs = PI * (nFH + nFL); //2 PI times required frequency shift (FHiCut+FLoCut)/2
  double fCenter = 0.5 * (double)(numCoeffs - 1); //floating point center index of FIR filter

  for (int i = 0; i < numCoeffs; i++) //zero pad entire coefficient buffer
  {
    coeffs_I[i] = 0.0;
    coeffs_Q[i] = 0.0;
  }

  //create LP FIR windowed sinc, sin(x)/x complex LP filter coefficients
  for (int i = 0; i < numCoeffs; i++)
  {
    double x = (float32_t)i - fCenter;
    double z;
    if ( abs((double)i - fCenter) < 0.01) //deal with odd size filter singularity where sin(0)/0==1
      z = 2.0 * nFc;
    else
      switch (FIR_filter_window) {
        case 1:    // 4-term Blackman-Harris --> this is what Power SDR uses
          z = (double)sin(TWO_PI * x * nFc) / (PI * x) *
              (0.35875 - 0.48829 * cos( (TWO_PI * i) / (numCoeffs - 1) )
               + 0.14128 * cos( (FOURPI * i) / (numCoeffs - 1) )
               - 0.01168 * cos( (SIXPI * i) / (numCoeffs - 1) ) );
          break;
        case 2:
          z = (double)sin(TWO_PI * x * nFc) / (PI * x) *
              (0.355768 - 0.487396 * cos( (TWO_PI * i) / (numCoeffs - 1) )
               + 0.144232 * cos( (FOURPI * i) / (numCoeffs - 1) )
               - 0.012604 * cos( (SIXPI * i) / (numCoeffs - 1) ) );
          break;
        case 3: // cosine
          z = (double)sin(TWO_PI * x * nFc) / (PI * x) *
              cos((PI * (float32_t)i) / (numCoeffs - 1));
          break;
        case 4: // Hann
          z = (double)sin(TWO_PI * x * nFc) / (PI * x) *
              0.5 * (double)(1.0 - (cos(PI * 2 * (double)i / (double)(numCoeffs - 1))));
          break;
        default: // Blackman-Nuttall window
          z = (double)sin(TWO_PI * x * nFc) / (PI * x) *
              (0.3635819
               - 0.4891775 * cos( (TWO_PI * i) / (numCoeffs - 1) )
               + 0.1365995 * cos( (FOURPI * i) / (numCoeffs - 1) )
               - 0.0106411 * cos( (SIXPI * i) / (numCoeffs - 1) ) );
          break;
      }
    //shift lowpass filter coefficients in frequency by (hicut+lowcut)/2 to form bandpass filter anywhere in range
    coeffs_I[i]  =  z * cos(nFs * x);
    coeffs_Q[i] = z * sin(nFs * x);
  }
}
 
void doConvolutionalInitialize(){

 /****************************************************************************************
     init complex FFTs
 ****************************************************************************************/
  
 S = &arm_cfft_sR_f32_len256;
 iS = &arm_cfft_sR_f32_len256;
 maskS = &arm_cfft_sR_f32_len256;
  
  /****************************************************************************************
     Calculate the FFT of the FIR filter coefficients once to produce the FIR filter mask
  ****************************************************************************************/
  
 init_filter_mask();
  /****************************************************************************************
     begin to queue the audio from the audio library
  ****************************************************************************************/
  Q_in_L.begin();
  Q_in_R.begin();
}

void reInitializeFilter(double dFLoCut, double dFHiCut){

  AudioNoInterrupts();
 /****************************************************************************************
     set filter bandwidth
  ****************************************************************************************/
  // this routine does all the magic of calculating the FIR coeffs
  calc_cplx_FIR_coeffs (FIR_Coef_I, FIR_Coef_Q, m_NumTaps, dFLoCut, dFHiCut, SAMPLE_RATE);
  /****************************************************************************************
     Calculate the FFT of the FIR filter coefficients once to produce the FIR filter mask
  ****************************************************************************************/
  init_filter_mask();

  AudioInterrupts();

}


/*- Execute the main convolutional processing, at the moment denoise spectral subtraction is active */
void doConvolutionalProcessing(float iNRLevel, boolean bFilterEnabled, double dFLoCut, double dFHiCut){
  
  // are there at least N_BLOCKS buffers in each channel available ?
    if (Q_in_L.available() > N_BLOCKS + 0 && Q_in_R.available() > N_BLOCKS + 0)
    {
      // get audio samples from the audio  buffers and convert them to float
      for (unsigned i = 0; i < N_BLOCKS; i++)
      {
        sp_L = Q_in_L.readBuffer();
        sp_R = Q_in_R.readBuffer();

        // convert to float one buffer_size
        // float_buffer samples are now standardized from > -1.0 to < 1.0
        arm_q15_to_float (sp_L, &float_buffer_L[BUFFER_SIZE * i], BUFFER_SIZE); // convert int_buffer to float 32bit
        arm_q15_to_float (sp_R, &float_buffer_R[BUFFER_SIZE * i], BUFFER_SIZE); // convert int_buffer to float 32bit
        Q_in_L.freeBuffer();
        Q_in_R.freeBuffer();
      }
 
      /**********************************************************************************
          Digital convolution
       **********************************************************************************/
      //  basis for this was Lyons, R. (2011): Understanding Digital Processing.
      //  "Fast FIR Filtering using the FFT", pages 688 - 694
      //  numbers for the steps taken from that source
      //  Method used here: overlap-and-save

      // ONLY FOR the VERY FIRST FFT: fill first samples with zeros
      if (first_block) // fill real & imaginaries with zeros for the first BLOCKSIZE samples
      {
        for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS; i++)
        {
          FFT_buffer[i] = 0.0;
        }
        first_block = 0;
      }
      else
      {  // HERE IT STARTS for all other instances
        // fill FFT_buffer with last events audio samples
        for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS; i++)
        {
          FFT_buffer[i * 2] = last_sample_buffer_L[i]; // real
          FFT_buffer[i * 2 + 1] = last_sample_buffer_R[i]; // imaginary
        }
      }
      // copy recent samples to last_sample_buffer for next time!
      for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS; i++)
      {
        last_sample_buffer_L [i] = float_buffer_L[i];
        last_sample_buffer_R [i] = float_buffer_R[i];
      }

      // now fill recent audio samples into FFT_buffer (left channel: re, right channel: im)
      for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS; i++)
      {
        FFT_buffer[FFT_length + i * 2] = float_buffer_L[i]; // real
        FFT_buffer[FFT_length + i * 2 + 1] = float_buffer_R[i]; // imaginary
      }

      /**********************************************************************************
          Complex Forward FFT
       **********************************************************************************/
      // calculation is performed in-place the FFT_buffer [re, im, re, im, re, im . . .]
      arm_cfft_f32(S, FFT_buffer, 0, 1);

     /* here we can process also the magnitude for general use ... 
     * Process the data through the Complex Magniture Module for calculating the magnitude at each bin */
     //arm_cmplx_mag_f32(FFT_buffer, FFTBufferMag, FFT_length);
     
     /**********************************************************************************
          Complex multiplication with filter mask (precalculated coefficients subjected to an FFT)
       **********************************************************************************/
    if (bFilterEnabled == true){   
       arm_cmplx_mult_cmplx_f32 (FFT_buffer, FIR_filter_mask, iFFT_buffer, FFT_length);
    }else{
       arm_copy_f32(FFT_buffer, iFFT_buffer, FFT_length);
    }
     
     /**********************************************************************************
          Complex inverse FFT
       **********************************************************************************/
      arm_cfft_f32(iS, iFFT_buffer, 1, 1);

      /**********************************************************************************
          Overlap and save algorithm, which simply means yóu take only the right part of the buffer and discard the left part
       **********************************************************************************/
        for (unsigned i = 0; i < FFT_length / 2; i++)
        {
          float_buffer_L[i] = iFFT_buffer[FFT_length + i * 2];
          float_buffer_R[i] = iFFT_buffer[FFT_length + i * 2 + 1];
        }

       /**********************************************************************************
          Demodulation / manipulation / do whatever you want 
       **********************************************************************************/
       //  at this time, just put filtered audio (interleaved format, overlap & save) into left and right channel     

       // apply the LMS but with single block.
       if ( iNRLevel >0){ 
         if (iNRLevel!=oldNRLevel){
            Init_LMS_NR (iNRLevel);
            oldNRLevel = iNRLevel;
         }
        
         LMS_NoiseReduction(128, float_buffer_L);
         for (int i = 0; i < BUFFER_SIZE; i++)
         {  float_buffer_L [i] = float_buffer_L [i]* 1.1;
            float_buffer_R [i] = float_buffer_L [i];
         }    
       }

       /**********************************************************************
          CONVERT TO INTEGER AND PLAY AUDIO - Push audio into I2S audio chain
       **********************************************************************/
      for (int i = 0; i < N_BLOCKS; i++)
        {
          sp_L = Q_out_L.getBuffer();    
          sp_R = Q_out_R.getBuffer();
          arm_float_to_q15 (&float_buffer_L[BUFFER_SIZE * i], sp_L, BUFFER_SIZE); 
          arm_float_to_q15 (&float_buffer_R[BUFFER_SIZE * i], sp_R, BUFFER_SIZE);
          Q_out_L.playBuffer(); // play it !  
          Q_out_R.playBuffer(); // play it !
        }

    } // end of audio process loop
}

#endif /* RDSP_CONVOLUTIONAL_H_INCLUDED */

/**************************************END OF FILE****/
