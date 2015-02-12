#ifndef _MISC_H_
#define _MISC_H_

extern void reboot();
extern void serialPrintf(const char *fmt, ...);

#define DEBUG false
#if DEBUG
#define DLOG(...) serialPrintf(__VA_ARGS__)
#else
#define DLOG(...) 0
#endif



#endif  // _MISC_H_

