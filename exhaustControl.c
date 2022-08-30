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
#include <limits.h>
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

struct queue {
	unsigned int tail;	    // current tail
	unsigned int head;	    // current head
	unsigned int size;	    // current number of items
	unsigned int capacity;      // Capacity of queue
	float* data; 		    // Pointer to array of data
};

// Create Global defenition of queue_t
typedef struct queue queue_t;

// NAME 	: create_queue()
// INPUT 	: _capacity : capacity of circular queue
// OUTPUT 	: NULL on failure.
// 		  pointer to created circular queue (queue_t*)
// DESCRIPTION	: determine if queue is empty
queue_t* create_queue(unsigned int _capacity){

	queue_t* myQueue = (queue_t*)malloc(sizeof(queue_t)); // allocate memory of size of queue struct

	if (myQueue == NULL ){
		return NULL; // if malloc was unsuccesful return NULL
	} else {
		// populate the variables of the queue :
		myQueue->tail = -1;
		myQueue->head = 0;
		myQueue->size = 0;
		myQueue->capacity = _capacity;
		myQueue->data = (float*)malloc(_capacity * sizeof(float)); // allocate memory for the array

		return myQueue;
	}
}

// NAME 	: queue_empty()
// INPUT 	: q : pointer to circular queue (queue_t*).
// OUTPUT 	: -1 if q is NULL
// 		  1 if q is empty
// 		  0 if q is not empty
// DESCRIPTION	: determine if queue is empty

int queue_empty(queue_t* q){
		if (q == NULL){
			return -1;
		}else if(q->size == 0) {
			return 1;
		}else {
			return 0;
		}
}

// NAME 	: queue_full()
// INPUT 	: q : pointer to circular queue (queue_t*).
// OUTPUT 	: -1 if q is NULL
// 		  1 if q is full
// 		  0 if q is not full
// DESCRIPTION	: determine if queue is full
int queue_full(queue_t* q){
	if (q == NULL){
		return -1;
	}else if(q->size == q->capacity){
		return 1;
	}else{
		return 0;
	}
}

// NAME 	:	queue_enqueue()
// INPUT 	: q : pointer to circular queue (queue_t*)
// 		  item : integer to be added to queue
// OUTPUT 	: -1 if q is NULL
// 		  1 if item was added successfully
// 		  0 otherwise
// DESCRIPTION	: Enqueue item into circular queue q.
int queue_enqueue(queue_t* q, float item){

		if (q == NULL){
			return -1;
		}	else if (queue_full(q) == 1){
			// make sure the queue isnt full.
			return 0;
		} else {
		// first we move the tail (insert) location up one (in the circle (size related to _capacity))
		q->tail = (q->tail + 1) % q->capacity; // this makes it go around in a circle
		// now we can add the actual item to the location
		q->data[q->tail] = item;
		// now we have to increase the size.
		q->size++;
		return 1;
		}
}

// NAME 	: queue_enqueue()
// INPUT 	: q : pointer to circular queue (queue_t*)
// OUTPUT 	: -1 if q is NULL
// 		  0 if q is Empty
//		  else returns value of item at front of line.
// DESCRIPTION	: Dequeue circular queue q, returning next value.
// Note : we are ASSUMING all values in q are greater than zero.
int queue_dequeue(queue_t *q){

		if (q == NULL){
			return -1;

		}	else if (queue_empty(q) == 1){
			return 0;
		}else{
			// firt capture the item
			 float item = q->data[q->head];
			 q->head = (q->head + 1) % q->capacity;
			 // decrease size by 1
			 q->size--;

			 return item;
		}
	}

// NAME 	: queue_size()
// INPUT 	: q : pointer to circular queue (queue_t*).
// OUTPUT 	: -1 if q is NULL
// 		  else return current size of circular queue q.
// DESCRIPTION	: determine size of queue q.
unsigned int queue_size(queue_t* q){
	if (q == NULL){
		return - 1;
	} else {
		return q->size;
	}
}

// NAME 	: free_queue()
// INPUT 	: q : pointer to circular queue (queue_t*).
// OUTPUT 	: NONE
// DESCRIPTION	: free memory associatioed with circular queue q
void free_queue(queue_t* q){
			// free the array
			free(q->data);
			// free queue
			free(q);
}

void print_queue(queue_t* q){
	for(int i=0; i < q->capacity; i++){
		printf("[%f] ", q->data[i]);
	}
	printf("\n");
}

bool allLow(queue_t* q){
	for(int i = 0; i<q->capacity; i++) {
		if(q->data[i] == HIGH) {
			return false;
		}
	}
	return true;
}

float queue_average(queue_t* q){
	float dataSum = 0.0;
	for(int i = 0; i<q->capacity; i++) {
		dataSum += q->data[i];
	}
	return dataSum/q->capacity;
}

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

	struct queue* buttonQueue = create_queue(20);
	struct queue* tpsQueue = create_queue(60);

	closeValve(fd); //Close the valve on start to get to a known state
	while(1) {
		if(queue_full(tpsQueue)) {
			queue_dequeue(tpsQueue);
		}	
		queue_enqueue(tpsQueue, read_adc_voltage(1, 0));
		if(queue_full(tpsQueue)) {
			tpsAvg = queue_average(tpsQueue);
		}
		if(queue_full(buttonQueue)) {
			queue_dequeue(buttonQueue);
		}
		queue_enqueue(buttonQueue, digitalRead(ButtonPin));
		if(allLow(buttonQueue)){
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




