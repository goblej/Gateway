#include "Particle.h"
#include "uart.h"
#include <cctype>

/****************************************************************************
  Section: Global variables
  ***************************************************************************/

void hexDumpPrint(char *buf)
{
	Serial.printf ( "%s\r\n", buf );
}

void hexDump(uint8_t *buf, uint16_t len)
{
	uint16_t i,j,k,n;
	uint8_t ob[80];
	
	/* Start with null terminated string of spaces */
	memset((void*)ob, ' ', sizeof(ob));
	ob[(sizeof(ob) - 1)] = 0x00;
	
	//    writeATTRsp("\r\n");
	
	/* Display 16 bytes per line */
	for(i = 0; i < len; i+=16)
	{
		k = sprintf((char*)ob, "   %03x: ", (unsigned int)i);
		
		if((i + 16) > len)
			n = (len - i);
		else 
			n = 16;
		
		for(j = 0; j < 16; j++)
		{
			if(j < n)
			{
				sprintf((char*)ob+k+(j*3), "%02X ", *(buf+i+j));
				sprintf((char*)ob+k+16*3+j+2, "%c", isprint(*(buf+i+j)) ? *(buf+i+j) : '.');
			}
			else
			{
				sprintf((char*)ob+k+(j*3), "   ");
				sprintf((char*)ob+k+16*3+j+2, " ");
			}
		}
	
		/* Pad to the end with spaces and null terminate */
		ob[+k+(16*3)] = ' ';
		ob[79] = 0x00;
		
		/* Print one line */
		hexDumpPrint((char*)ob);
	}

	Serial.write ("\r\n" );
}


