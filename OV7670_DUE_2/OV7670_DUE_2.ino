//
// Source code for application to transmit image from ov7670 to PC via USB
// Example for Arduino Due By Siarhei Charkes in 2016
// 
// http://privateblog.info 
//
// ReferÃªncia:
// http://privateblog.info/arduino-due-i-kamera-ov7670-primer-ispolzovaniya/
//
// QCIF  176 Ã— 144

#include <Wire.h>
#include <SPI.h>
#include "OV7670_DUE_2.h"

// PWM
const int freq1 = 21000000;
const int freq2 = 21000000;
const int pin = 6;
const int minfreq1 = 641;
// fim PWM

extern uint8_t myImage[RESOLUTION_V][RESOLUTION_H];

void setup() 
{
 // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  pinMode(24, INPUT_PULLUP ); 
  
  //Serial.begin(460800);
  SerialUSB.begin(1000000);
  
  Wire.begin();
   
//  int32_t mask_PWM_pin = digitalPinToBitMask(7);
//  REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
//  REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
//  REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B    
//  REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
//  REG_PWM_CMR6 = 0<<9;             // select clock and polarity for PWM channel (pin7) -> (CPOL = 0)
//  REG_PWM_CPRD6 =  8;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
//  REG_PWM_CDTY6 = 4;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
//  REG_PWM_ENA = 1<<6;               // enable PWM on PWM channel (pin 7 = PWML6)
  pinMode(6,OUTPUT);
  analogWriteResolution(12);
  pmc_enable_periph_clk(PWM_INTERFACE_ID);
  //Configure Clocks
  PWMC_ConfigureClocks(freq1,freq2,2*freq1);
  Set_PWM(7,8000000,2);

  camInit();
  
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(75);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(500);
  }

  if (RESOLUTION_H == 320 && RESOLUTION_V == 240)
    setRes_qvga();
  else 
    setRes_min_pix();
    
  setColor();
  write(0x11, 5); // CLKRC: Era 6 // Bit[6]:  0: Apply prescaler on input clock 1: Use external clock directly

  pinMode(25, INPUT);
  pinMode(26, INPUT);
  pinMode(27, INPUT);
  pinMode(28, INPUT);
  pinMode(13, INPUT);
  pinMode(15, INPUT);
  pinMode(29, INPUT);
  pinMode(11, INPUT);
  
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(250);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(250);
  }
  
}


void loop()
{
  if (!digitalRead(24))
  {
    captureImg(RESOLUTION_H, RESOLUTION_V);
  }
}

static void captureImg(uint16_t wg, uint16_t hg){
  uint16_t y, x;

  while (!(REG_PIOB_PDSR & (1 << 21)));     // 21 : VSYNC wait for high
  while ((REG_PIOB_PDSR & (1 << 21)));      // 21 : VSYNC wait for low

  y = hg;
  while (y--)
  {
    x = wg;
    while (x--)
    {
      while ((REG_PIOD_PDSR & (1 << 10)));          // D32 : PCLK wait for low
     // myImage[y][x] = (REG_PIOC_PDSR & 0xFF000) >> 12; // 
      myImage[y][x] = REG_PIOD_PDSR;
      while (!(REG_PIOD_PDSR & (1 << 10)));         // D32: wait for high PCLK
      while ((REG_PIOD_PDSR & (1 << 10)));          //wait for low
      while (!(REG_PIOD_PDSR & (1 << 10)));         //wait for high      
    }
  }
  
  /*
   *  c/ Presc. (0x11) de 0x05 tempo de 3uS de PCLK.
   * 
   * 
   */
   
// Serial.println("*RDY*");
  SerialUSB.print("*RDY*");
 
  digitalWrite(LED_BUILTIN, HIGH);    
  for (y = 0; y < hg; y++) 
  {
    for (x = 0; x < wg; x++) 
    { 
      if(PRETO_E_BRANCO == 1)  
      {  
        if (myImage[y][x] <= 68)
          myImage[y][x] = 0;
        else
          myImage[y][x] = 0xFF;
      } 
//      uart_putchar(myImage[y][x]); 
     SerialUSB.write(myImage[y][x]);
    }
    
  }
  SerialUSB.print("*STP*");
  digitalWrite(LED_BUILTIN, LOW); 
  delay(3);
}


static inline int uart_putchar(const uint8_t c) {
    while(!(UART->UART_SR & UART_SR_TXRDY));
    UART->UART_THR = c;
    return 0;
}

void Set_PWM(int pin,int frequency,int clock)
{
    //Configure Pin
  PIO_Configure(g_APinDescription[pin].pPort,
  g_APinDescription[pin].ulPinType,
  g_APinDescription[pin].ulPin,
  g_APinDescription[pin].ulPinConfiguration);
  //Set Pin to count off CLKA set at freq1
  int chan = g_APinDescription[pin].ulPWMChannel;
  int clk = clock == 1 ? PWM_CMR_CPRE_CLKA : PWM_CMR_CPRE_CLKB;
  PWMC_ConfigureChannel(PWM_INTERFACE,chan,clk,0,0);
  int divider = 42000000/frequency;
  PWMC_SetPeriod(PWM_INTERFACE,chan,divider);
  PWMC_SetDutyCycle(PWM_INTERFACE,chan,divider/2);
  PWMC_EnableChannel(PWM_INTERFACE,chan);

}
/* 
static void captureImg(uint16_t wg, uint16_t hg){
  uint16_t y, x;

 
  while (!(REG_PIOB_PDSR & (1 << 21)));     // 21 : VSYNC wait for high
  while ((REG_PIOB_PDSR & (1 << 21)));      // 21 : VSYNC wait for low

  y = hg;
  while (y--){
    x = wg;
    while (x--){
      while ((REG_PIOD_PDSR & (1 << 10)));          // 10 : PCLK wait for low
      myImage[y][x] = (REG_PIOC_PDSR & 0xFF000) >> 12; 
      while (!(REG_PIOD_PDSR & (1 << 10)));         //wait for high
      while ((REG_PIOD_PDSR & (1 << 10)));          //wait for low
      while (!(REG_PIOD_PDSR & (1 << 10)));         //wait for high      
    }
  }
  
// Serial.println("*RDY*");
  SerialUSB.print("*RDY*");
 
  digitalWrite(LED_BUILTIN, HIGH);    
  for (y = 0; y < hg; y++) 
  {
    for (x = 0; x < wg; x++) 
    { 
      if(PRETO_E_BRANCO == 1)  
      {  
        if (myImage[y][x] <= 68)
          myImage[y][x] = 0;
        else
          myImage[y][x] = 0xFF;
      } 
//      uart_putchar(myImage[y][x]); 
     SerialUSB.write(myImage[y][x]);
    }
    
  }
  SerialUSB.print("*STP*");
  digitalWrite(LED_BUILTIN, LOW); 
}

*/

