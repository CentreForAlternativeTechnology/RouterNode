#include "RTC.h"
#include <avr/eeprom.h>
#define eeprom_read(a) eeprom_read_byte((unsigned char *) a)
#define eeprom_write(a, v) eeprom_write_byte((unsigned char *) a, v)
Time::Time(unsigned int year, unsigned int month,
  unsigned int date, unsigned int day,
  unsigned int hours, unsigned int minutes,
  unsigned int seconds){
  this->year = year;
  this->month = month;
  this->date = date;
  this->day = day;
  this->hours = hours;
  this->minutes = minutes;
  this->seconds = seconds;
}
Time::Time(){}
//Time takes up 7 bytes - because we don't care for space too much
void Time::loadFromEEPROM(int address){
  this->year = (unsigned int)eeprom_read(address) + 2000;
  this->month = (unsigned int)eeprom_read(address+1);
  this->date = (unsigned int)eeprom_read(address+2);
  this->day = (unsigned int)eeprom_read(address+3);
  this->hours = (unsigned int)eeprom_read(address+4);
  this->minutes = (unsigned int)eeprom_read(address+5);
  this->seconds = (unsigned int)eeprom_read(address+6);
}
void Time::saveToEEPROM(int address){
  eeprom_write(address, (uint8_t)(this->year-2000));
  eeprom_write(address+1, (uint8_t)this->month);
  eeprom_write(address+2, (uint8_t)this->date);
  eeprom_write(address+3, (uint8_t)this->day);
  eeprom_write(address+4, (uint8_t)this->hours);
  eeprom_write(address+5, (uint8_t)this->minutes);
  eeprom_write(address+6, (uint8_t)this->seconds);
}
unsigned int Time::getHours(){
  return this->hours;
}
unsigned int Time::getMinutes(){
  return this->minutes;
}
unsigned int Time::getSeconds(){
  return this->seconds;
}
unsigned int Time::getDate(){
  return this->date;
}
unsigned int Time::getDay(){
  return this->day;
}
unsigned int Time::getMonth(){
  return this->month;
}
unsigned int Time::getYear(){
  return this->year;
}
void Time::setYear(unsigned int year){
  this->year = year;
}
void Time::setMonth(unsigned int month){
  this->month = month;
}
void Time::setDate(unsigned int date){
  this->date = date;
}
void Time::setDay(unsigned int day){
  this->day = day;
}
void Time::setHours(unsigned int hours){
  this->hours = hours;
}
void Time::setMinutes(unsigned int minutes){
  this->minutes = minutes;
}
void Time::setSeconds(unsigned int seconds){
  this->seconds = seconds;
}
int Time::compare(Time other){
  int rv = 0;
  if((rv = other.getYear()-this->getYear())!=0){
    return rv;
  }
  if((rv = other.getMonth()-this->getMonth())!=0){
    return rv;
  }
  if((rv = other.getDate()-this->getDate())!=0){
    return rv;
  }
  if((rv = other.getHours()-this->getHours())!=0){
    return rv;
  }
  if((rv = other.getMinutes()-this->getMinutes())!=0){
    return rv;
  }
  if((rv = other.getSeconds()-this->getSeconds())!=0){
    return rv;
  }
  return rv;
}
void Time::clone(Time *t){
  if(t!=NULL){
    t->setYear(this->year);
    t->setMonth(this->month);
    t->setDate(this->date);
    t->setDay(this->day);
    t->setHours(this->hours);
    t->setMinutes(this->minutes);
    t->setSeconds(this->seconds);
  }
}
