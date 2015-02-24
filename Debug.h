#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG
#ifdef DEBUG

#ifdef LINUX
#include <string>
#include <ostream>
#include <iostream>
#define LOG(x) std::cout << (x)
#ifndef F
#define F(x) x
#endif
#else
#define LOG(x) Serial.print(x)
#include "Arduino.h"
#endif

#else
#define LOG(x)
#endif

#endif