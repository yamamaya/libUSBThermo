// USB thermography library for OTK-THG01/02

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "usbhid.h"
#include "usbthermo.h"

#undef DEBUG

#define PACKETSIZE	64

struct USBThermo_ {
	HidDevice *hid;
};

static __attribute__ ((unused)) void Dump( u_char *data, int len ) {
	int i;
	for ( i = 0 ; i < len ; i ++ ) {
		if ( i % 16 == 0 ) {
			if ( i ) {
				printf( "\n" );
			}
			printf( "%04x:", i );
		}
		printf( " %02x", (int)data[i] );
	}
	printf( "\n" );
}

int USBThermoCommand( USBThermo *dev, u_char *dest, u_char cmd1, u_char cmd2, u_char cmd3, u_char cmd4 ) {
	u_char buff[PACKETSIZE+1];

	memset( buff, 0, sizeof( buff ) );
	memset( dest, 0, PACKETSIZE );

	buff[0] = 0;
	buff[1] = cmd1;
	buff[2] = cmd2;
	buff[3] = cmd3;
	buff[4] = cmd4;
#ifdef DEBUG
	printf( "HidWrite %d bytes\n", PACKETSIZE );
	Dump( buff + 1, PACKETSIZE );
	printf( "... " );
#endif
	int res = HidWrite( dev->hid, buff, PACKETSIZE + 1 );
	if (res < 0) {
#ifdef DEBUG
		printf( "Unable to write()\n" );
#endif
		return 0;
	}
#ifdef DEBUG
	printf( "OK\n" );
#endif

        res = 0;
        while ( res == 0 ) {
#ifdef DEBUG
		printf( "HidRead ... " );
#endif
		res = HidRead( dev->hid, dest, PACKETSIZE );
		if ( res == 0 ) {
#ifdef DEBUG
			printf( "waiting...\n" );
#endif
		} else if ( res < 0 ) {
#ifdef DEBUG
			printf( "Unable to read()\n" );
#endif
			return 0;
		} else {
#ifdef DEBUG
			printf( "OK, %d bytes read\n", res );
			Dump( dest, res );
#endif
			break;
		}
		usleep( 10*1000 );
	}

	return 1;
}

void USBThermoInit( void ) {
	HidInit();
}

void USBThermoExit( void ) {
	HidExit();
}

USBThermo *USBThermoOpen( void ) {
	HidDevice *hid = HidOpen( 0x04d8, 0xfa87 );
	if ( ! hid ) {
		return NULL;
	}

	USBThermo *dev = calloc( 1, sizeof( USBThermo ) );
	dev->hid = hid;
	return dev;
}

void USBThermoClose( USBThermo *dev ) {
	HidClose( dev->hid );
	free( dev );
}

static uint16_t unsigned16( const unsigned char *data, u_int index ) {
	uint16_t d = data[index] | ( (uint16_t)data[index+1] << 8 );
	return (uint16_t)d;
}

static int16_t signed16( const unsigned char *data, u_int index ) {
	int32_t d = (int32_t)( data[index] | ( (uint32_t)data[index+1] << 8 ) );
	if ( d && 0x8000 ) {
		d = d - 65536;
	}
	return (int16_t)d;
}

static void ParseHalfFrame( const unsigned char *rawdata, USBThermoPixel *dest ) {
	int y;
	for ( y = 0 ; y < 2 ; y ++ ) {
		int x;
		for ( x = 0 ; x < 16 ; x ++ ) {
			int16_t t = signed16( rawdata, 0 );
			if ( t == -9991 || t == -9992 ) {
				dest->status = USBThermo_Underflow;
				dest->temperature = -50;
			} else if ( t == -9990 ) {
				dest->status = USBThermo_Overflow;
				dest->temperature = 300;
			} else {
				dest->status = USBThermo_Normal;
				dest->temperature = (double)t / 10;
			}
			dest ++;
			rawdata += 2;
		}
	}
}

int USBThermoRead( USBThermo *dev, USBThermoFrame *frame ) {
	unsigned char buff1[PACKETSIZE];
	unsigned char buff2[PACKETSIZE];

	frame->width = USBThermo_Width;
	frame->height = USBThermo_Height;
	int res = USBThermoCommand( dev, buff1, 0x00, 0x00, 0x00, 0x00 );
	if ( ! res || buff1[0] != 0x01 ) {
		return 0;
	}
	res = USBThermoCommand( dev, buff1, 0x01, 0x00, 0x00, 0x00 );
	if ( ! res ) {
		return 0;
	}
	res = USBThermoCommand( dev, buff2, 0x01, 0x01, 0x00, 0x00 );
	if ( ! res ) {
		return 0;
	}

	ParseHalfFrame( buff1, frame->pixel[0] );
	ParseHalfFrame( buff2, frame->pixel[2] );

	return 1;
}

int USBThermoSetEmissivity( USBThermo *dev, double emissivity ) {
	if ( emissivity <= 0 || emissivity > 1 ) {
		return 0;
	}
	uint16_t e = (uint16_t)( emissivity * 1000 + 0.5 );
	unsigned char buff[PACKETSIZE];
	int res = USBThermoCommand( dev, buff, 0x04, (u_char)( e & 0xff ), (u_char)( e >> 8 ), 0x00 );
	if ( ! res || buff[0] != 0x01 ) {
		return 0;
	}
	return 1;
}

int USBThermoSetRefreshRate( USBThermo *dev, USBThermoRefreshRate refresh ) {
	unsigned char buff[PACKETSIZE];
	int res = USBThermoCommand( dev, buff, 0x03, (u_char)refresh, 0x00, 0x00 );
	if ( ! res || buff[0] != 0x01 ) {
		return 0;
	}
	return 1;
}

int USBThermoReadRawData( USBThermo *dev, USBThermoRawData *rawdata ) {
	unsigned char buff[PACKETSIZE];
	int res = USBThermoCommand( dev, buff, 0x82, 0x00, 0x00, 0x00 );
	if ( ! res ) {
		return 0;
	}
	rawdata->PTAT = unsigned16( buff, 0 );
	rawdata->Vcp = signed16( buff, 2 );

	res = USBThermoCommand( dev, buff, 0x81, 0x00, 0x00, 0x00 );
	if ( ! res ) {
		return 0;
	}
	int x, y;
	int i = 0;
	for ( y = 0 ; y < 2 ; y ++ ) {
		for ( x = 0 ; x < 16 ; x ++ ) {
			rawdata->Vir[y][x] = signed16( buff, i );
			i += 2;
		}
	}

	res = USBThermoCommand( dev, buff, 0x81, 0x01, 0x00, 0x00 );
	if ( ! res ) {
		return 0;
	}
	i = 0;
	for ( y = 2 ; y < 4 ; y ++ ) {
		for ( x = 0 ; x < 16 ; x ++ ) {
			rawdata->Vir[y][x] = signed16( buff, i );
			i += 2;
		}
	}

	return 1;
}

int USBThermoReadEEPROM( USBThermo *dev, USBThermoEEPROM *eeprom ) {
	eeprom->size = 256;
	int page;
	unsigned char buff[PACKETSIZE];
	int i = 0;
	for ( page = 0 ; page < 4 ; page ++ ) {
		int res = USBThermoCommand( dev, buff, 0x80, page, 0x00, 0x00 );
		if ( ! res ) {
			return 0;
		}
		int j;
		for ( j = 0 ; j < 64 ; j ++ ) {
			eeprom->data[i] = buff[j];
			i ++;
		}
	}

	return 1;
}
