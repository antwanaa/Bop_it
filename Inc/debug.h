#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stdio.h>

#ifndef DEBUG_ENABLE
#define DEBUG_ENABLE    1
#endif


#define DEBUG_MAIN_FILE             1
#define DEBUG_FREERTOS_FILE         1
#define DEBUG_AUDIO_PLAYBACK_FILE   0



/* Macro ------------------------------------------------------------*/
//! define DEBUG_THIS_FILE in each files you want to print
#define debugf(...)     do { if (DEBUG_THIS_FILE && DEBUG_ENABLE) printf(__VA_ARGS__); } while (0)


#endif