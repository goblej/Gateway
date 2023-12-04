#ifndef BUILD_H
#define BUILD_H

/****************************************************************************
 * Includes
 ****************************************************************************/
#include <cstdint>

/*****************************************************************************
 * Software version
 *****************************************************************************/
#define SW_VERSION_MAJOR            97
#define SW_VERSION_MINOR            3
#define SW_VERSION_PATCH            2

/*****************************************************************************
 * Function enablers
 *****************************************************************************/
#define CLI_EDIT
#define CMD_HISTORY_SIZE	8
#define MAX_CMDLINE			50

#define MAX_EVENT_LENGTH 128
#define TRANSFER_BUFFER_SIZE	128
#define BLOCK_TRANSFER_HEADER	12

/* When the M524 becomes available */
/* This opens up a few more uart baud rate and framng options */
/* Necessary for some panel types */
#define M524

/* eventMessage_t */
/* Event Structure used for each buffered event */
typedef struct
{
	union{
		struct{
			uint8_t  Type;			/* Event type, GPIO, Advanced, Gent, Siemens etc    */
			uint8_t  Length [ 3 ];	/* Number of bytes to follow    */
			uint32_t TimeStamp;		/* Seconds since January 1, 1970   */
			uint32_t SecondFracPart;	/* Fractional seconds (just set to 0) */
			uint8_t  Data [ 244 ];	/* Payload */
		};
		struct{
      /* All of the above */
			uint8_t Event [ 256 ];
		};
	};
} eventMessage_t;

/* Structure of Nimbus transfer message */
typedef struct
{
	union{
		struct{
			uint8_t  Type;				/* Command Type, always 0x83 for event transfers */
			uint8_t  Length [ 3 ];		/* Number of bytes to follow    */
			uint32_t uniqueTransferId;	/* Rolling Message ID */
			uint32_t TimeStamp;			/* Seconds since January 1, 1970 */
			uint32_t SecondFracPart;		/* Fractional seconds (just set to 0) */
			uint8_t  Data [ 1008];	/* Payload */
		};
		struct{
      /* All of the above */
			uint8_t Message [ 1024 ];
		};
	};
} nimbusTransferMessage_t;

/*****************************************************************************/

#endif /* BUILD_H */
