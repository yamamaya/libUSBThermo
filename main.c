#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "usbthermo.h"

// USB thermography library for OTK-THG01/02 sample program

int main(int argc, char* argv[]) {
	setbuf( stdout, NULL );

	// Initialize library
	USBThermoInit();

	// Open the device
	printf( "USBThermoOpen ... " );
	USBThermo *dev = USBThermoOpen();
	if ( ! dev ) {
		printf( "unable to open device\n" );
 		return 1;
	}
	printf( "OK\n" );

	// Set emissivity
	USBThermoSetEmissivity( dev, 0.95 );
	// Set refresh rate
	USBThermoSetRefreshRate( dev, USBThermoRefreshRate_2Hz );

	// Read thermal image
	printf( "USBThermoRead:\n" );
	USBThermoFrame frame;
	USBThermoRead( dev, &frame );
	int x, y;
	for ( y = 0 ; y < frame.height ; y ++ ) {
		for ( x = 0 ; x < frame.width ; x ++ ) {
			printf( "%02.1f ", frame.pixel[y][x].temperature );
		}
		printf( "\n" );
	}
	printf( "\n" );

	// Read raw data from the sensor
	printf( "USBThermoReadRawData:\n" );
	USBThermoRawData rawdata;
	USBThermoReadRawData( dev, &rawdata );
	printf( "PTAT: %u\n", rawdata.PTAT );
	printf( "Vcp: %d\n", rawdata.Vcp );
	printf( "Vir:\n" );
	for ( y = 0 ; y < 4 ; y ++ ) {
		for ( x = 0 ; x < 16 ; x ++ ) {
			printf( "%+05d ", rawdata.Vir[y][x] );
		}
		printf( "\n" );
	}
	printf( "\n" );

	// Read data from EEPROM in the sensor
	printf( "USBThermoReadEEPROM:\n" );
	USBThermoEEPROM eeprom;
	USBThermoReadEEPROM( dev, &eeprom );
	int i;
	for ( i = 0 ; i < eeprom.size ; i ++ ) {
		printf( "%02x ", eeprom.data[i] );
		if ( i % 16 == 15 ) {
			printf( "\n" );
		}
	}
	printf( "\n" );

	// Close the device
	USBThermoClose( dev );

	// Clean up the library
	USBThermoExit();

	return 0;
}
