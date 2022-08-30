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

typedef struct Queued_Float QueuedFloat;

struct Queued_Float
{
    int          value;
    QueuedFloat *nextFloat;
};

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


int add_float_to_queue(float newValue, QueuedFloat **lastFloat)
{
    QueuedFloat *newQueuedFloat;

    // Create a new action in memory
    if ((newQueuedFloat = (QueuedAction *)malloc(sizeof(QueuedAction))) == NULL)
        return 0;

    // Make the old 'lastAction' point to the new Action, 
    // and the new Action to point to NULL:
    *lastAction -> nextAction = newQueuedAction;
    newQueuedAction -> nextAction = NULL;
    newQueuedAction -> action = newAction;
    newQueuedAction -> value = newValue;

    // Designate the new Action as the new lastAction:
    *lastAction = newQueuedAction;
    return 1;
}













// Data structure to represent a stack
struct floatStack {
    int maxsize;    // define max capacity of the stack
    int top;
    float *items;
};

// Data structure to represent a stack
struct intStack {
    int maxsize;    // define max capacity of the stack
    int top;
    int *items;
};



// Utility function to initialize the stack
struct floatStack* newFloatStack(int capacity) {
    struct floatStack *pt = (struct floatStack*)malloc(sizeof(struct floatStack));
 
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = (float*)malloc(sizeof(float) * capacity);
 
    return pt;
}

// Utility function to initialize the stack
struct intStack* newIntStack(int capacity) {
    struct intStack *pt = (struct intStack*)malloc(sizeof(struct intStack));
 
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = (int*)malloc(sizeof(int) * capacity);
 
    return pt;
}

// Utility function to return the size of the stack
int intSize(struct intStack *pt) {
    return pt->top + 1;
}

// Utility function to return the size of the stack
int floatSize(struct floatStack *pt) {
    return pt->top + 1;
}

float floatStructAvg(struct floatStack *pt) {
	float sum = 0.0;
	for(int i=0; i<pt->maxsize; i++) {
		sum += pt->items[i];
	}
	return sum/pt->maxsize;
}

bool allLow(struct intStack *pt) {

	for(int i=0; i<pt->maxsize; i++) {
		printf("[%d] ", pt->items[i]);
		if(pt->items[i] == HIGH) {
		//	return false;
		}
	}
	printf("\n");	//return true;
	return false;
}

// Utility function to check if the stack is empty or not
int floatIsEmpty(struct floatStack *pt) {
    return pt->top == -1;                   // or return size(pt) == 0;
}

// Utility function to check if the stack is empty or not
int intIsEmpty(struct intStack *pt) {
    return pt->top == -1;                   // or return size(pt) == 0;
}


// Utility function to check if the stack is full or not
int floatIsFull(struct floatStack *pt) {
    return pt->top == pt->maxsize - 1;      // or return size(pt) == pt->maxsize;
}

// Utility function to check if the stack is full or not
int intIsFull(struct intStack *pt) {
    return pt->top == pt->maxsize - 1;      // or return size(pt) == pt->maxsize;
}


// Utility function to add an element `x` to the stack
void floatPush(struct floatStack *pt, float x) {
    // check if the stack is already full. Then inserting an element would
    // lead to stack overflow
    if (floatIsFull(pt)) {
        printf("Overflow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }

    // add an element and increment the top's index
    pt->items[++pt->top] = x;
}

// Utility function to add an element `x` to the stack
void intPush(struct intStack *pt, int x) {
    // check if the stack is already full. Then inserting an element would
    // lead to stack overflow
    if (intIsFull(pt)) {
        printf("Overflow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }

    // add an element and increment the top's index
    pt->items[++pt->top] = x;
}

// Utility function to pop a top element from the stack
float floatPop(struct floatStack *pt) {
    // check for stack underflow
    if (floatIsEmpty(pt)) {
        printf("Underflow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }

    // decrement stack size by 1 and (optionally) return the popped element
    return pt->items[pt->top--];
}

// Utility function to pop a top element from the stack
int intPop(struct intStack *pt) {
    // check for stack underflow
    if (intIsEmpty(pt)) {
        printf("Underflow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }

    // decrement stack size by 1 and (optionally) return the popped element
    int item = pt->items[0];
    pt->items[0] = pt->items[1];
    pt->top--;
    return item;
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
	float tpsAvg;
	tpsAvg = 0.0;
	bool isOpen;
	isOpen = false;
	bool manualOperation;
	manualOperation = false;
	int tpsThreshold;
	tpsThreshold = 2.8;
	pinMode(ButtonPin, INPUT);
	pullUpDnControl(ButtonPin, PUD_UP);

	struct floatStack *tpsVals = newFloatStack(60);
	struct intStack *buttonVals = newIntStack(20);
	
	closeValve(fd); //Close the valve on start to get to a known state
	while(1) {
		if(floatIsFull(tpsVals)) {
			floatPop(tpsVals);
			tpsAvg = floatStructAvg(tpsVals);
		}	
		floatPush(tpsVals, read_adc_voltage(1, 0));
		if(intIsFull(buttonVals)) {
			intPop(buttonVals);
		}
		intPush(buttonVals, digitalRead(ButtonPin));
		
		if(allLow(buttonVals)){
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
		if(tpsAvg > tpsThreshold && !manualOperation && !isOpen) {
			openValve(fd);
			isOpen=true;
		} else if (tpsAvg <= tpsThreshold && !manualOperation && isOpen) {
			closeValve(fd);
			isOpen=false;
		}
	}
	return 0;
}




