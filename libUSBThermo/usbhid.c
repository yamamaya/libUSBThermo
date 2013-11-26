// USB HID helper library for libUSBThermo

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libusb.h"
#include "usbhid.h"

struct HidDevice_ {
	libusb_device_handle *device_handle;
	
	// Endpoint information
	int input_endpoint;
	int output_endpoint;
	int input_ep_max_packet_size;

	// The interface number of the HID
	int interface;

	// Defualt timeout
	int timeout_read;
	int timeout_write;
};

static int initialized = 0;

static HidDevice *new_HidDevice( void ) {
	HidDevice *dev = calloc( 1, sizeof( HidDevice ) );
	dev->timeout_read = 5000;
	dev->timeout_write = 5000;
	
	return dev;
}

static void free_HidDevice( HidDevice *dev ) {
	free( dev );
}

int HidInit( void ) {
	if ( ! initialized ) {
		if ( libusb_init( NULL ) ) {
			return -1;
		}
		initialized = 1;
	}

	return 0;
}

int HidExit( void ) {
	if ( initialized ) {
		libusb_exit( NULL );
		initialized = 0;
	}

	return 0;
}

static char *MakePath( libusb_device *dev, int interface_number ) {
	char str[64];
	snprintf(
		str, sizeof( str ),
		"%04x:%04x:%02x",
		libusb_get_bus_number( dev ),
		libusb_get_device_address( dev ),
		interface_number
	);
	str[sizeof( str )-1] = 0;
	
	return strdup( str );
}

HidDevicePathList *HidEnumerate( uint16_t vendor_id, uint16_t product_id ) {
	
	HidDevicePathList *root = NULL;
	HidDevicePathList *cur_dev = NULL;
	
	if ( ! initialized ) {
		HidInit();
	}

	libusb_device **devs;
	ssize_t num_devs = libusb_get_device_list( NULL, &devs );
	if ( num_devs < 0 ) {
		return NULL;
	}
	int i = 0;
	libusb_device *dev;
	while ( ( dev = devs[i] ) != NULL ) {
		i ++;

		struct libusb_device_descriptor desc;
		int res = libusb_get_device_descriptor( dev, &desc );
		uint16_t dev_vid = desc.idVendor;
		uint16_t dev_pid = desc.idProduct;
		
		if ( desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE && ( vendor_id == 0 || vendor_id == dev_vid ) && ( product_id == 0 || product_id == dev_pid ) ) {

			struct libusb_config_descriptor *conf_desc = NULL;
			res = libusb_get_active_config_descriptor( dev, &conf_desc );
			if ( res < 0 ) {
				libusb_get_config_descriptor( dev, 0, &conf_desc );
			}
			if ( conf_desc ) {
				int j;
				for ( j = 0 ; j < conf_desc->bNumInterfaces ; j++ ) {
					const struct libusb_interface *intf = &conf_desc->interface[j];
					int k;
					for ( k = 0 ; k < intf->num_altsetting ; k++ ) {
						const struct libusb_interface_descriptor *intf_desc = &intf->altsetting[k];
						if ( intf_desc->bInterfaceClass == LIBUSB_CLASS_HID ) {
							HidDevicePathList *tmp = calloc( 1, sizeof( HidDevicePathList ) );
							if ( cur_dev ) {
								cur_dev->next = tmp;
							} else {
								root = tmp;
							}
							cur_dev = tmp;

							cur_dev->path = MakePath( dev, intf_desc->bInterfaceNumber );
							cur_dev->next = NULL;
							break;
						}
					}
				}
				libusb_free_config_descriptor( conf_desc );
			}
		}
	}

	libusb_free_device_list( devs, 1 );

	return root;
}

void HidFreeEnumeration( HidDevicePathList *devs ) {
	HidDevicePathList *d = devs;
	while ( d ) {
		HidDevicePathList *next = d->next;
		free( d->path );
		free( d );
		d = next;
	}
}

HidDevice *HidOpen( uint16_t vendor_id, uint16_t product_id ) {
	HidDevicePathList *devs;
	HidDevice *handle;
	
	devs = HidEnumerate( vendor_id, product_id );
	if ( devs == NULL ) {
		return NULL;
	}

	handle = HidOpenPath( devs->path );

	HidFreeEnumeration( devs );
	
	return handle;
}

HidDevice *HidOpenPath( const char *path ) {
	HidDevice *dev = new_HidDevice();

	libusb_device **devs;
	libusb_device *usb_dev;
	int res;
	int good_open = 0;
	
	if ( ! initialized ) {
		HidInit();
	}

	libusb_get_device_list( NULL, &devs );
	int d = 0;
	while ( ( usb_dev = devs[d] ) != NULL ) {
		d++;

		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor( usb_dev, &desc );

		struct libusb_config_descriptor *conf_desc = NULL;
		if ( libusb_get_active_config_descriptor( usb_dev, &conf_desc ) < 0 ) {
			continue;
		}
		int j;
		for ( j = 0 ; j < conf_desc->bNumInterfaces ; j++ ) {
			const struct libusb_interface *intf = &conf_desc->interface[j];
			int k;
			for ( k = 0 ; k < intf->num_altsetting ; k++ ) {
				const struct libusb_interface_descriptor *intf_desc;
				intf_desc = &intf->altsetting[k];
				if ( intf_desc->bInterfaceClass == LIBUSB_CLASS_HID ) {
					char *dev_path = MakePath( usb_dev, intf_desc->bInterfaceNumber );
					if ( ! strcmp( dev_path, path ) ) {
						/* Matched Paths. Open this device */

						res = libusb_open( usb_dev, &dev->device_handle );
						if ( res < 0 ) {
							free( dev_path );
 							break;
						}
						
						/* Detach the kernel driver, but only if the device is managed by the kernel */
						if ( libusb_kernel_driver_active( dev->device_handle, intf_desc->bInterfaceNumber ) == 1 ) {
							res = libusb_detach_kernel_driver( dev->device_handle, intf_desc->bInterfaceNumber );
							if ( res < 0 ) {
								libusb_close( dev->device_handle );
								free( dev_path );
								break;
							}
						}
						
						res = libusb_claim_interface( dev->device_handle, intf_desc->bInterfaceNumber );
						if ( res < 0 ) {
							free( dev_path );
							libusb_close( dev->device_handle );
							break;
						}

						/* Store off the interface number */
						dev->interface = intf_desc->bInterfaceNumber;
												
						/* Find the INPUT and OUTPUT endpoints. An OUTPUT endpoint is not required. */
						int i;
						for ( i = 0; i < intf_desc->bNumEndpoints ; i++ ) {
							const struct libusb_endpoint_descriptor *ep = &intf_desc->endpoint[i];

							/* Determine the type and direction of this endpoint. */
							int is_interrupt = (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT;
							int is_output = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
							int is_input = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;

							/* Decide whether to use it for intput or output. */
							if (dev->input_endpoint == 0 && is_interrupt && is_input) {
								/* Use this endpoint for INPUT */
								dev->input_endpoint = ep->bEndpointAddress;
								dev->input_ep_max_packet_size = ep->wMaxPacketSize;
							}
							if (dev->output_endpoint == 0 && is_interrupt && is_output) {
								/* Use this endpoint for OUTPUT */
								dev->output_endpoint = ep->bEndpointAddress;
							}
						}

						good_open = 1;
						
					}
					free( dev_path );
				}
			}
		}
		libusb_free_config_descriptor( conf_desc );

	}

	libusb_free_device_list( devs, 1 );
	
	// If we have a good handle, return it.
	if (good_open) {
		return dev;
	} else {
		// Unable to open any devices.
		free_HidDevice( dev );
		return NULL;
	}
}

void HidSetDefaultTimeout( HidDevice *device, int milliseconds ) {
	device->timeout_write = milliseconds;
	device->timeout_read = milliseconds;
}

int HidWriteTimeout( HidDevice *device, const u_char *data, size_t length, int milliseconds ) {
	int report_number = data[0];
	int skipped_report_id = 0;
	if ( report_number == 0x00 ) {
		data++;
		length--;
		skipped_report_id = 1;
	}

	int transferred;
	int res = libusb_interrupt_transfer(
		device->device_handle,
		device->output_endpoint,
		(u_char *)data,
		length,
		&transferred,
		milliseconds
	);
	
	if ( res < 0 ) {
		return -1;
	}
		
	if ( skipped_report_id ) {
		transferred ++;
	}
		
	return transferred;
}

int HidWrite( HidDevice *device, const u_char *data, size_t length ) {
	return HidWriteTimeout( device, data, length, device->timeout_write );
}

int HidReadTimeout( HidDevice *device, u_char *data, size_t length, int milliseconds ) {
	int transferred;
	int res = libusb_interrupt_transfer( device->device_handle, device->input_endpoint, data, length, &transferred, milliseconds);
	if ( res < 0 ) {
		return res;
	}
	return transferred;
}

int HidRead( HidDevice *device, u_char *data, size_t length ) {
	return HidReadTimeout( device, data, length, device->timeout_read );
}

void HidClose( HidDevice *device ) {
	if ( ! device ) {
		return;
	}
	
	libusb_release_interface( device->device_handle, device->interface );
	libusb_close( device->device_handle );
	free_HidDevice( device );
}
