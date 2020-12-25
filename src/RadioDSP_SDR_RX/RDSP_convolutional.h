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

#define        STATING_BIN_VAD_ANALISYS 60
#define        ENDING_BIN_VAD_ANALISYS  120

#define        BUFFER_SIZE 128
float          TH_VALUE = 0.09; 
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
float32_t      FFTBufferMag [FFT_L * 2] __attribute__ ((aligned (4))); 
float32_t      float_buffer_L [BUFFER_SIZE * N_B];  // 2048 * 4 = 8kb
float32_t      float_buffer_R [BUFFER_SIZE * N_B]; // 2048 * 4 = 8kb

float32_t      FFT_buffer [FFT_L * 2] __attribute__ ((aligned (4)));  // 4096 * 4 = 16kb
float32_t      last_sample_buffer_L [BUFFER_SIZE * N_B];  // 2048 * 4 = 8kb
float32_t      last_sample_buffer_R [BUFFER_SIZE * N_B]; // 2048 * 4 = 8kb

// This is only Dummy buffer . Mybe we don't need this, but I need it for compile pourposes. TO BE CHECKED !
float32_t      FIR_filter_mask [FFT_L * 2] __attribute__ ((aligned (4)));  // 4096 * 4 = 16kb
//*************************************************************************************************
//*************************************************************************************************

//************************************************************************
//************************************************************************
//*******************   CONVOLUTIONAL SECTION  ***************************
//************************************************************************
 
void doConvolutionalInitialize(){

  /****************************************************************************************
     init complex FFTs
  ****************************************************************************************/
  switch (FFT_length)
  {
    case 256:
      S = &arm_cfft_sR_f32_len256;
      iS = &arm_cfft_sR_f32_len256;
      break;
    case 512:
      S = &arm_cfft_sR_f32_len512;
      iS = &arm_cfft_sR_f32_len512;
      break;
    case 1024:
      S = &arm_cfft_sR_f32_len1024;
      iS = &arm_cfft_sR_f32_len1024;
      break;
    case 2048:
      S = &arm_cfft_sR_f32_len2048;
      iS = &arm_cfft_sR_f32_len2048;
      break;
    case 4096:
      S = &arm_cfft_sR_f32_len4096;
      iS = &arm_cfft_sR_f32_len4096;
      break;
  }

 // This is only Dummy buffer . Mybe we don't need this, but I need it for compile pourposes. TO BE CHECKED !
 arm_cfft_f32(S, FIR_filter_mask , 0, 1);
  /****************************************************************************************
     begin to queue the audio from the audio library
  ****************************************************************************************/
  Q_in_L.begin();
  Q_in_R.begin();
}

void doConvolutionalProcessing_Denoise(){
  
 // elapsedMicros usec = 0;
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

      /* Process the data through the Complex Magniture Module for calculating the magnitude at each bin */
      arm_cmplx_mag_f32(FFT_buffer, FFTBufferMag, FFT_length);

      // ONLY TEST !!!!!!
      float specVal = 0.0;
      if(TH_VALUE>0){
        for(int m = STATING_BIN_VAD_ANALISYS; m<=ENDING_BIN_VAD_ANALISYS; m++)  { specVal = specVal+FFTBufferMag[m]; }
        // Evaluate mean value...
        TH_VALUE = specVal/(ENDING_BIN_VAD_ANALISYS - STATING_BIN_VAD_ANALISYS);
        TH_VALUE = TH_VALUE*3;
      }

      /* Substraction ... */
      for (int j =0; j< FFT_length*2; j++)
      {
        /*- Subtract threshold noise */
       if (FFTBufferMag[j]<=TH_VALUE){
            FFTBufferMag[j] = FFTBufferMag[j] * 0.2;
        }else{
            FFTBufferMag[j] =  FFTBufferMag[j]- TH_VALUE;
        }
      }

      /* Rebuild & discard the fft complex signal from subctracted magnitude & original signal phase */
      int k =1;
      int s= 0;
      for (int j =0; j< FFT_length*2; j++)
      {
    
          float32_t r1 = FFT_buffer[s]; float32_t i1 = FFT_buffer[k];
          //complex float z= r1 + i1*I;
          //float32_t phi = cargf(z);
          float32_t phi = atan2(i1, r1);
          
          float32_t r2 = FFTBufferMag[j]*arm_cos_f32(phi);
          float32_t i2 = FFTBufferMag[j]*arm_sin_f32(phi);
    
          iFFT_buffer[s] = r2;
          iFFT_buffer[k] = i2;
          s=s+2;
          k=s+1;
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
