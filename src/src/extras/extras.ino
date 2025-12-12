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

#include "Pixel.h"

 WS2801_LED *p;
//WS2801_LED::Color empty[8]={};
//WS2801_LED::Color colors[8]={WS2801_LED::RGB(0, 50, 0),  WS2801_LED::RGB(50, 50, 0), WS2801_LED::RGB(50, 0, 0) ,WS2801_LED::RGB(0, 0, 0), WS2801_LED::RGB(0, 0, 255), WS2801_LED::RGB(0,0,0), WS2801_LED::RGB(0,0,255),WS2801_LED::RGB(255,0,0)};

void setup() {

  Serial.begin(115200);
  delay(2000);
  Serial.printf("\n\nReady %d %d\n",F14,F22);
  p=new WS2801_LED(F14, F22);
}

void loop() {
  p->set(WS2801_LED::RGB(255,0,0),25);
  delay(1000);
  p->set(WS2801_LED::RGB(0,0,255),25);
  delay(1000);    
}
