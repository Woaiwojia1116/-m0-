#ifndef __KET_H
#define __KET_H
#include <stdint.h>
#include "ti_msp_dl_config.h"
typedef struct
{
	uint8_t keyState;
	uint8_t state;
	uint8_t singleFlag;

}Key;
void clickKey (void);
extern Key key[3];
#endif