#!/bin/sh

rm -f usbthermo-test libUSBThermo/usbhid.o libUSBThermo/usbthermo.o main.o

cc -Wall -g -c -I. -IlibUSBThermo `pkg-config libusb-1.0 --cflags` libUSBThermo/usbhid.c -o libUSBThermo/usbhid.o
cc -Wall -g -c -I. -IlibUSBThermo libUSBThermo/usbthermo.c -o libUSBThermo/usbthermo.o
cc -Wall -g -c -I. -IlibUSBThermo main.c -o main.o

cc -Wall -g libUSBThermo/usbhid.o libUSBThermo/usbthermo.o main.o `pkg-config libusb-1.0 --libs` -o usbthermo-test
