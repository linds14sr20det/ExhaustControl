/*
 * exhaustControl.c
 *
 *  Created on: 25 Aug 2022
 *
 *      compile with "gcc ABE_ADCDACPi.c exhaustControl.c -lwiringPi -o exhaustControl"
 *      run with "./exhaustControl"
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "ABE_ADCDACPi.h"

#define DEVICE_ADDR  0x10
#define RELAY1  0x01
#define RELAY2  0x02
#define RELAY3  0x03
#define RELAY4  0x04
#define ON      0xFF
#define OFF     0x00

#define LedPin    0
#define ButtonPin 1

void openValve(int fd) {
	wiringPiI2CWriteReg8(fd, 1, ON);
	sleep(1);
	wiringPiI2CWriteReg8(fd, 1, OFF);
	return;
}

void closeValve(int fd) {
	wiringPiI2CWriteReg8(fd, 2, ON);
	sleep(1);
	wiringPiI2CWriteReg8(fd, 2, OFF);
	return;
}

int main(void) {
	if(wiringPiSetup() == -1) { 
		printf("setup wiringPi failed !\n");
		return -1; 
	}

	if (open_adc() != 1) { // open the ADC spi channel
		printf("opening adc failed\n");
		exit(1); // if the SPI bus fails to open exit the program
	}

	int fd;
	fd = wiringPiI2CSetup(DEVICE_ADDR);	
	float tpsVoltage;
	tpsVoltage = 0.0;
	bool isOpen;
	isOpen = false;
	bool manualOperation;
	manualOperation = false;
	int tpsThreshold;
	tpsThreshold = 2.8;
	pinMode(ButtonPin, INPUT);
	pullUpDnControl(ButtonPin, PUD_UP);
	
	while(1) {
		tpsVoltage = read_adc_voltage(1, 0);
		if(digitalRead(ButtonPin) == LOW){
	        	if(isOpen) {
				closeValve(fd);
				isOpen=false;
				manualOperation = false;
			} else {
				openValve(fd);
				isOpen=true;
				manualOperation=true;
			}
	       	}
		if(tpsVoltage > tpsThreshold && !manualOperation && !isOpen) {
			openValve(fd);
			isOpen=true;
		} else if (tpsVoltage <= tpsThreshold && !manualOperation && isOpen) {
			closeValve(fd);
			isOpen=false;
		}
	}
	return 0;
}




