/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2020-2024 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
////////////////////////////////////////////
//           Addressable LEDs             //
////////////////////////////////////////////

#include "Pixel.h"

////////////////////////////////////////////
//     Single-Wire RGB/RGBW NeoPixels     //
////////////////////////////////////////////

Pixel::Pixel(int pin, const char *pixelType){
    
  this->pin=pin;

  rmt_tx_channel_config_t tx_chan_config;
  tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;                       // always use 80MHz clock source
  tx_chan_config.gpio_num = (gpio_num_t)pin;                          // GPIO number
  tx_chan_config.mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL;   // set number of symbols to match those in a single channel block
  tx_chan_config.resolution_hz = 80 * 1000 * 1000;                    // set to 80MHz
  tx_chan_config.intr_priority = 3;                                   // medium interrupt priority
  tx_chan_config.trans_queue_depth = 4;                               // set the number of transactions that can pend in the background
  tx_chan_config.flags.invert_out = false;                            // do not invert output signal
  tx_chan_config.flags.with_dma = false;                              // use RMT channel memory, not DMA (most chips do not support use of DMA anyway)
  tx_chan_config.flags.io_loop_back = false;                          // do not use loop-back mode
  tx_chan_config.flags.io_od_mode = false;                            // do not use open-drain output
  
  if(!GPIO_IS_VALID_OUTPUT_GPIO(pin)){
    ESP_LOGE(PIXEL_TAG,"Can't create Pixel(%d) - invalid output pin",pin);
    return;    
  }

  if(rmt_new_tx_channel(&tx_chan_config, &tx_chan)!=ESP_OK){
    ESP_LOGE(PIXEL_TAG,"Can't create Pixel(%d) - no open channels",pin);
    return;
  }
  
  bytesPerPixel=0;
  size_t len=strlen(pixelType);
  boolean invalidMap=false;
  char v[]="RGBWC01234-";                                 // list of valid mapping characters for pixelType
  
  for(int i=0;i<len && i<5;i++){                          // parse and then validate pixelType
    int index=strchrnul(v,toupper(pixelType[i]))-v;
    if(index==strlen(v))                                  // invalid mapping character found
      invalidMap=true;
    map[bytesPerPixel++]=index%5;                         // create pixel map and compute number of bytes per pixel
  }

  if(bytesPerPixel<3 || len>5 || invalidMap){
    ESP_LOGE(PIXEL_TAG,"Can't create Pixel(%d, \"%s\") - invalid pixelType",pin,pixelType);
    return;
  }

  sscanf(pixelType,"%ms",&pType);                 // save pixelType for later use with hasColor()
  
  rmt_enable(tx_chan);                            // enable channel
  channel=((int *)tx_chan)[0];                    // get channel number

  tx_config.loop_count=0;                         // populate tx_config structure
  tx_config.flags.eot_level=0;
  tx_config.flags.queue_nonblocking=0;
  
  rmt_bytes_encoder_config_t encoder_config;          // can leave blank for now - will populate from within setTiming() below
  rmt_new_bytes_encoder(&encoder_config, &encoder);   // create byte encoder
   
  setTiming(0.32, 0.88, 0.64, 0.56, 80.0);    // set default timing parameters (suitable for most SK68 and WS28 RGB pixels)

  onColor.HSV(0,100,100,0);
}

///////////////////

Pixel *Pixel::setTiming(float high0, float low0, float high1, float low1, uint32_t lowReset){

  if(channel<0)
    return(this);
  
  rmt_bytes_encoder_config_t encoder_config;

  encoder_config.bit0.level0=1;
  encoder_config.bit0.duration0=high0*80+0.5;
  encoder_config.bit0.level1=0;
  encoder_config.bit0.duration1=low0*80+0.5;
  
  encoder_config.bit1.level0=1;
  encoder_config.bit1.duration0=high1*80+0.5;
  encoder_config.bit1.level1=0;
  encoder_config.bit1.duration1=low1*80+0.5;

  encoder_config.flags.msb_first=1;                               // MSB of data bytes should be converted and transmitted first
  
  rmt_bytes_encoder_update_config(encoder,&encoder_config);       // update config
  
  resetTime=lowReset;
  return(this);
}

///////////////////

void Pixel::set(Color *c, int nPixels, boolean multiColor){

  if(channel<0 || nPixels==0)
    return;

  Color data[2];      // temp ping/pong structure to store re-mapped color bytes
  int index=0;        // points to current slot in ping/pong structure

  rmt_ll_set_group_clock_src(&RMT, channel, RMT_CLK_SRC_DEFAULT, 1, 0, 0);        // ensure use of DEFAULT CLOCK, which is always 80 MHz, without any scaling
  
  do {
    for(int i=0;i<bytesPerPixel;i++)                                              // remap colors into ping/pong structure
      data[index].col[i]=c->col[map[i]];

    rmt_tx_wait_all_done(tx_chan,-1);                                             // wait until any outstanding data is transmitted
    rmt_transmit(tx_chan, encoder, data[index].col, bytesPerPixel, &tx_config);   // transmit data
    
    index=1-index;                                                                // flips index to second data structure
    c+=multiColor;
  } while(--nPixels>0);

  rmt_tx_wait_all_done(tx_chan,-1);                                               // wait until final data is transmitted
  delayMicroseconds(resetTime);                                                   // end-of-marker delay
}

////////////////////////////////////////////
//          Two-Wire RGB DotStars         //
////////////////////////////////////////////

Dot::Dot(uint8_t dataPin, uint8_t clockPin){

  pinMode(dataPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  digitalWrite(dataPin,LOW);
  digitalWrite(clockPin,LOW);

  dataMask=1<<(dataPin%32);
  clockMask=1<<(clockPin%32);

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #define OUT_W1TS  &GPIO.out_w1ts.val
  #define OUT_W1TC  &GPIO.out_w1tc.val
  #define OUT1_W1TS  NULL
  #define OUT1_W1TC  NULL
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define OUT_W1TS  &GPIO.out_w1ts.val
  #define OUT_W1TC  &GPIO.out_w1tc.val
  #define OUT1_W1TS  &GPIO.out1_w1ts.val
  #define OUT1_W1TC  &GPIO.out1_w1tc.val
#else
  #define OUT_W1TS  &GPIO.out_w1ts
  #define OUT_W1TC  &GPIO.out_w1tc
  #define OUT1_W1TS  &GPIO.out1_w1ts.val
  #define OUT1_W1TC  &GPIO.out1_w1tc.val
#endif

  dataSetReg=     dataPin<32 ? (OUT_W1TS) : (OUT1_W1TS);
  dataClearReg=   dataPin<32 ? (OUT_W1TC) : (OUT1_W1TC);
  clockSetReg=    clockPin<32 ? (OUT_W1TS) : (OUT1_W1TS);
  clockClearReg=  clockPin<32 ? (OUT_W1TC) : (OUT1_W1TC);
}

///////////////////

void Dot::set(Color *c, int nPixels, boolean multiColor){
  
  *dataClearReg=dataMask;           // send all zeros
  for(int j=0;j<31;j++){
    *clockSetReg=clockMask;
    *clockClearReg=clockMask;    
  }
  
  for(int i=0;i<nPixels;i++){
    for(int b=31;b>=0;b--){
      if((c->val>>b)&1)
        *dataSetReg=dataMask;
      else
        *dataClearReg=dataMask;
      *clockSetReg=clockMask;
      *clockClearReg=clockMask;
    }
    c+=multiColor;
  }

  *dataClearReg=dataMask;           // send all zeros
  for(int j=0;j<31;j++){
    *clockSetReg=clockMask;
    *clockClearReg=clockMask;    
  }
}

////////////////////////////////////////////
