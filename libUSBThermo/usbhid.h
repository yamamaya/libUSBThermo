// USB HID helper library for libUSBThermo

#ifndef __USBHID_H__
#define __USBHID_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct HidDevice_;
typedef struct HidDevice_ HidDevice;

typedef struct HidDevicePathList_ HidDevicePathList;
struct HidDevicePathList_ {
	char *path;
	HidDevicePathList *next;
};

int HidInit( void );

int HidExit( void );

HidDevice *HidOpen( uint16_t vendor_id, uint16_t product_id );
HidDevice *HidOpenPath( const char *path );

void HidClose( HidDevice *device );

HidDevicePathList *HidEnumerate( uint16_t vendor_id, uint16_t product_id );
void HidFreeEnumeration( HidDevicePathList *devs );

void HidSetDefaultTimeout( HidDevice *device, int milliseconds );

int HidWrite( HidDevice *device, const u_char *data, size_t length );
int HidWriteTimeout( HidDevice *device, const u_char *data, size_t length, int milliseconds );

int HidRead( HidDevice *device, u_char *data, size_t length );
int HidReadTimeout( HidDevice *device, u_char *data, size_t length, int milliseconds );

#ifdef __cplusplus
}
#endif

#endif

