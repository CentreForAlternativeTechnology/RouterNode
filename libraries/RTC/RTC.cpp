#include "RTC.h"
// DS1302 RTC
// ----------
// A library for interfacing with the DS1302 real-time clock
// Based upon http://playground.arduino.cc/Main/DS1302 by "Krodal"
// Open Source / Public Domain
//
// By Tim Stableford(http://tstableford.co.uk)
//

// Designate pins sclk, io, and ce
RTC::RTC(uint8_t sclk, uint8_t io, uint8_t ce){
  this->ds_sclk = sclk;
  this->ds_io = io;
  this->ds_ce = ce;
}

// Update the internal buffer from the chip
void RTC::read(){
  int i;
  RTC::RTCStart();
  // Instead of the address,
  // the CLOCK_BURST_READ command is issued
  // the I/O-line is released for the data
  RTC::RTCToggleWrite( DS1302_CLOCK_BURST_READ, true);
  uint8_t *p = (uint8_t *)(&this->ds_time);
  for(i=0; i<8; i++){
    *p++ = RTC::RTCToggleRead();
  }
  RTC::RTCStop();
  //Copy the times from the structure to the Time object
  this->time.setHours((unsigned int)bcd2bin(this->ds_time.Hour10, this->ds_time.Hour));
  this->time.setMinutes((unsigned int)bcd2bin(this->ds_time.Minutes10, this->ds_time.Minutes));
  this->time.setSeconds((unsigned int)bcd2bin(this->ds_time.Seconds10, this->ds_time.Seconds));
  this->time.setDate((unsigned int)bcd2bin(this->ds_time.Date10, this->ds_time.Date));
  this->time.setMonth((unsigned int)bcd2bin(this->ds_time.Month10, this->ds_time.Month));
  this->time.setDay((unsigned int)(this->ds_time.Day));
  this->time.setYear(2000 + (unsigned int)(bcd2bin(this->ds_time.Year10, this->ds_time.Year)));
}
Time *RTC::getDateTime(){
  return &time;
}
//Checks if params are acceptable, if so sets the trickle state, else disables it
//The values of the diodes and resistors are on the chip
//Acceptable - 1/2 Diodes, Resistor val 2K, 4K, 8K
void RTC::setTrickleState(int num_diodes, int resistor_val){
  struct trickle_struct trickle;
  switch(resistor_val){
    case 2:
      trickle.resistor_val = 0x01;
      trickle.enabled = 0x05;
      break;
    case 4:
      trickle.resistor_val = 0x02;
      trickle.enabled = 0x05;
      break;
    case 8:
      trickle.resistor_val = 0x03;
      trickle.enabled = 0x05;
      break;
    default:
      trickle.enabled = 0x00;
      break; 
  }
  if(trickle.enabled&&(num_diodes==1||num_diodes==2)){
    trickle.num_diodes = num_diodes;
  }else{
    trickle.enabled = 0x00;
  }
  //Odd workaround to allow trickle_struct cast to uint8_t
  RTC::RTCWrite(DS1302_TRICKLE, *((uint8_t *)(&trickle)));
}

void RTC::setDateTime() {
	this->setDateTime(&(this->time));
}

void RTC::setDateTime(Time *t){
  //Allow us to write
  this->RTCWrite(DS1302_ENABLE, 0);
  //Convert the time to the structures format
  this->ds_time.Seconds = bin2bcd_l(t->getSeconds());
  this->ds_time.Seconds10 = bin2bcd_h(t->getSeconds());
  this->ds_time.Minutes = bin2bcd_l(t->getMinutes());
  this->ds_time.Minutes10 = bin2bcd_h(t->getMinutes());
  this->ds_time.Hour = bin2bcd_l(t->getHours());
  this->ds_time.Hour10 = bin2bcd_h(t->getHours());
  this->ds_time.hour_12_24 = 0; // 0 for 24 hour format
  this->ds_time.Date = bin2bcd_l(t->getDate());
  this->ds_time.Date10 = bin2bcd_h(t->getDate());
  this->ds_time.Month = bin2bcd_l(t->getMonth());
  this->ds_time.Month10 = bin2bcd_h(t->getMonth());
  this->ds_time.Day = (uint8_t)t->getDay();
  this->ds_time.Year = bin2bcd_l(t->getYear()-2000);
  this->ds_time.Year10 = bin2bcd_h(t->getYear()-2000); 
  this->ds_time.WP = 0;
  this->ds_time.CH = 0;
  //Write the structure to chip
  this->RTCBurstWrite();

  this->read();
}

//We only need to free resources which we've malloced, which is none.
RTC::~RTC(){}

// --------------------------------------------------------
// DS1302_clock_burst_write
//
// This function writes 8 bytes clock data in burst mode
// to the DS1302.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void RTC::RTCBurstWrite(){
  int i;
  uint8_t *p = (uint8_t *)(&this->ds_time);

  RTC::RTCStart();
  // Instead of the address,
  // the CLOCK_BURST_WRITE command is issued.
  // the I/O-line is not released
  RTC::RTCToggleWrite(DS1302_CLOCK_BURST_WRITE, false);  

  for(i=0; i<8; i++){
    // the I/O-line is not released
    RTC::RTCToggleWrite(*p++, false);  
  }
  RTC::RTCStop();
}


// --------------------------------------------------------
// DS1302_read
//
// This function reads a byte from the DS1302
// (clock or ram).
//
// The address could be like "0x80" or "0x81",
// the lowest bit is set anyway.
//
// This function may be called as the first function,
// also the pinMode is set.
//
uint8_t RTC::RTCRead(int address)
{
  uint8_t data;

  // set lowest bit (read bit) in address
  bitSet(address, DS1302_READBIT);  

  RTC::RTCStart();
  // the I/O-line is released for the data
  RTC::RTCToggleWrite( address, true);  
  data = RTC::RTCToggleRead();
  RTC::RTCStop();

  return (data);
}


// --------------------------------------------------------
// DS1302_write
//
// This function writes a byte to the DS1302 (clock or ram).
//
// The address could be like "0x80" or "0x81",
// the lowest bit is cleared anyway.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void RTC::RTCWrite(int address, uint8_t data)
{
  // clear lowest bit (read bit) in address
  bitClear( address, DS1302_READBIT);  

  RTC::RTCStart();
  // don't release the I/O-line
  RTC::RTCToggleWrite( address, false);
  // don't release the I/O-line
  RTC::RTCToggleWrite( data, false);
  RTC::RTCStop();  
}

// --------------------------------------------------------
// _DS1302_start
//
// A helper function to setup the start condition.
//
// An 'init' function is not used.
// But now the pinMode is set every time.
// That's not a big deal, and it's valid.
// At startup, the pins of the Arduino are high impedance.
// Since the DS1302 has pull-down resistors,
// the signals are low (inactive) until the DS1302 is used.
void RTC::RTCStart(void)
{
  digitalWrite( this->ds_ce, LOW); // default, not enabled
  pinMode( this->ds_ce, OUTPUT);  

  digitalWrite( this->ds_sclk, LOW); // default, clock low
  pinMode( this->ds_sclk, OUTPUT);

  pinMode( this->ds_io, OUTPUT);

  digitalWrite( this->ds_ce, HIGH); // start the session
  delayMicroseconds( 4);           // tCC = 4us
}


// --------------------------------------------------------
// _DS1302_stop
//
// A helper function to finish the communication.
//
void RTC::RTCStop(void)
{
  // Set CE low
  digitalWrite( this->ds_ce, LOW);

  delayMicroseconds( 4);           // tCWH = 4us
}


// --------------------------------------------------------
// _DS1302_toggleread
//
// A helper function for reading a byte with bit toggle
//
// This function assumes that the SCLK is still high.
//
uint8_t RTC::RTCToggleRead(void)
{
  uint8_t i, data;

  data = 0;
  for( i = 0; i <= 7; i++)
  {
    // Issue a clock pulse for the next databit.
    // If the 'togglewrite' function was used before
    // this function, the SCLK is already high.
    digitalWrite(this->ds_sclk, HIGH);
    delayMicroseconds( 1);

    // Clock down, data is ready after some time.
    digitalWrite(this->ds_sclk, LOW);
    delayMicroseconds(1);        // tCL=1000ns, tCDD=800ns

    // read bit, and set it in place in 'data' variable
    bitWrite(data, i, digitalRead(this->ds_io));
  }
  return(data);
}


// --------------------------------------------------------
// _DS1302_togglewrite
//
// A helper function for writing a byte with bit toggle
//
// The 'release' parameter is for a read after this write.
// It will release the I/O-line and will keep the SCLK high.
//
void RTC::RTCToggleWrite( uint8_t data, uint8_t release)
{
  int i;

  for( i = 0; i <= 7; i++){
    // set a bit of the data on the I/O-line
    digitalWrite(this->ds_io, bitRead(data, i));  
    delayMicroseconds(1);     // tDC = 200ns

    // clock up, data is read by DS1302
    digitalWrite(this->ds_sclk, HIGH);    
    delayMicroseconds(1);     // tCH = 1000ns, tCDH = 800ns

    if(release && i == 7){
      // If this write is followed by a read,
      // the I/O-line should be released after
      // the last bit, before the clock line is made low.
      // This is according the datasheet.
      // I have seen other programs that don't release
      // the I/O-line at this moment,
      // and that could cause a shortcut spike
      // on the I/O-line.
      pinMode(this->ds_io, INPUT);

      // For Arduino 1.0.3, removing the pull-up is no longer needed.
      // Setting the pin as 'INPUT' will already remove the pull-up.
      // digitalWrite (DS1302_IO, LOW); // remove any pull-up  
    }else{
      digitalWrite(this->ds_sclk, LOW);
      delayMicroseconds(1);       // tCL=1000ns, tCDD=800ns
    }
  }
}
