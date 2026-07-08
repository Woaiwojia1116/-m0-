#include "key.h"

Key key[3] = {0};

void clickKey (void)
{
	key[0].keyState=!DL_GPIO_readPins(KEY_PIN_8_PORT,KEY_PIN_8_PIN);//按下为0,此时keystate为1
	key[1].keyState=!DL_GPIO_readPins(KEY_PIN_9_PORT,KEY_PIN_9_PIN);//按下为0,此时keystate为1
	key[2].keyState=!DL_GPIO_readPins(KEY_PIN_10_PORT,KEY_PIN_10_PIN);//按下为0,此时keystate为1
	for(int i=0;i<3;i++)
	{
		switch(key[i].state)
		{
			case 0:
			if(key[i].keyState)key[i].state=1;
				break;
			case 1:
			if(key[i].keyState)key[i].state=2;
			else
			{
				key[i].state=0;
			}
			break;
			case 2:
				if(!key[i].keyState)
				{
					key[i].state=0;
					key[i].singleFlag=1;
				}
				break;
		}
	}
}
