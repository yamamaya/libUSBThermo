// USB thermography library for OTK-THG01/02

#ifndef __USBTHERMO_H__
#define __USBTHERMO_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define USBThermo_Width		16
#define USBThermo_Height	4
#define USBThermo_EEPROM_Size	256

// Device handler
struct USBThermo_;
typedef struct USBThermo_ USBThermo;

// Pixel status
typedef enum {
	USBThermo_Normal = 0,
	USBThermo_Underflow,
	USBThermo_Overflow
} USBThermoPixelStatus;

// Refresh rate
typedef enum {
	USBThermoRefreshRate_0_5Hz = 0,
	USBThermoRefreshRate_1Hz = 1,
	USBThermoRefreshRate_2Hz = 2,
	USBThermoRefreshRate_4Hz = 4,
	USBThermoRefreshRate_8Hz = 8,
	USBThermoRefreshRate_16Hz = 16,
	USBThermoRefreshRate_32Hz = 32
} USBThermoRefreshRate;

// Pixel of thermal image
typedef struct {
	USBThermoPixelStatus status;
	double temperature;
} USBThermoPixel;

// Frame data of thermal image
typedef struct {
	int width;
	int height;
	USBThermoPixel pixel[USBThermo_Height][USBThermo_Width];
} USBThermoFrame;

// Raw data from sensor
typedef struct {
	uint16_t PTAT;
	int16_t Vcp;
	int16_t Vir[USBThermo_Height][USBThermo_Width];
} USBThermoRawData;

// EEPROM data in sensor
typedef struct {
	size_t size;
	u_char data[USBThermo_EEPROM_Size];
} USBThermoEEPROM;

// Initialize library
void USBThermoInit( void );
// Clean up library
void USBThermoExit( void );

// Open device
USBThermo *USBThermoOpen( void );
// Close device
void USBThermoClose( USBThermo *dev );

// Set emissivity
int USBThermoSetEmissivity( USBThermo *dev, double emissivity );
// Set refresh rate
int USBThermoSetRefreshRate( USBThermo *dev, USBThermoRefreshRate refresh );
// Read thermal image into USBThermoFrame
int USBThermoRead( USBThermo *dev, USBThermoFrame *frame );
// Read raw data into USBThermoRawData
int USBThermoReadRawData( USBThermo *dev, USBThermoRawData *rawdata );
// Read EEPROM data into USBThermoEEPROM
int USBThermoReadEEPROM( USBThermo *dev, USBThermoEEPROM *eeprom );

// Send command to device, "dest" should be u_char[64]
int USBThermoCommand( USBThermo *dev, u_char *dest, u_char cmd1, u_char cmd2, u_char cmd3, u_char cmd4 );

#ifdef __cplusplus
}
#endif

#endif

