#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG
#ifdef DEBUG
#define LOG(x) Serial.print(x)
#else
#define LOG(x)
#endif

#endif __DEBUG_H__