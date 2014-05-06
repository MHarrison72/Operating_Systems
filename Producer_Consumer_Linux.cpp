/*
* Author: Michael Harrison
* Multiple Producer-Consumer Problem : Linux
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <pthread.h>
#include <semaphore.h> //must compile with -lpthread
#include <errno.h>
#include <iostream>

using namespace std;

#define N 100
#define numT 6

int buff[N], front, rear;
sem_t full, empty;
pthread_mutex_t depositLock, fetchLock;
pthread_t *producers = new pthread_t[numT];
pthread_t *consumers = new pthread_t[numT];

void *prodRandInt(void *arg);
void *consRandInt(void *arg);

int main(void){
	int i = 0;
	front = 0; rear = 0;

	//clear buffer
	for(i = 0; i < N; i++){
		buff[i] = 0;
	}

	//initalize mutexes and semaphores
	pthread_mutex_init(&depositLock, NULL);
	pthread_mutex_init(&fetchLock, NULL);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, N);

	//create threads
	for(i = 0; i < numT; i++){
		pthread_create(&producers[i], NULL, prodRandInt, NULL);
		printf("Created Producer thread #%d\n", i);
		pthread_create(&consumers[i], NULL, consRandInt, NULL);
		printf("Created Consumer thread #%d\n", i);
	}

	//wait for threads to finish executing
	for(i = 0; i < numT; i++){
		pthread_join(producers[i], NULL);
		pthread_join(consumers[i], NULL);
	}

	//delete mutexes and semaphores
	pthread_mutex_destroy(&depositLock);
	pthread_mutex_destroy(&fetchLock);
	sem_destroy(&full);
	sem_destroy(&empty);

	return 0;
}

/*
* Output: printing a string of thread ID, number deposited, and position deposited to
* Assumptions: Will check empty semaphore to determine if buffer has free slot, then try and obtain mutex to deposit.
*        Deposits data in buffer at position rear and increments rear to next open slot.
*	     Gives up mutex for deposit, then increments full semaphore indicating successful deposit
*/
void *prodRandInt(void *arg){
	int depVal;

	while(1){
		depVal = rand(); //create data to be deposited

		sem_wait(&empty);	//empty slot?
		pthread_mutex_lock(&depositLock); //mutual exclusion for deposit

		buff[rear] = depVal; //deposit data
		rear = (rear % (N-1)) + 1;

		printf("Producer #%lu deposited %d & rear = %d\n", pthread_self(), depVal, rear);

		pthread_mutex_unlock(&depositLock);
		sem_post(&full); //increase full semaphore

		sleep(rand()%5);
	}

}

/*
* Output: printing a string of thread ID, number fetched, and position fetched from
* Assumptions: Will check full semaphore to determine if buffer has data, then try and obtain mutex to fetch.
*        Fetches data from buffer at position front and increments front to next filled slot.
*	     Gives up mutex for fetch, then increments empty semaphore indicating successful fetch
*/
void *consRandInt(void *arg){
	int fetVal;

	while(1){
		sem_wait(&full);		//filled slot?
		pthread_mutex_lock(&fetchLock);	//mutual exculsion for consume
		
		fetVal = buff[front]; //fetch data to be consumed
		front = (front % (N-1)) + 1;

		printf("Consumer #%lu fetched %d & front = %d\n", pthread_self(), fetVal, front);

		pthread_mutex_unlock(&fetchLock);
		sem_post(&empty); //increase empty semaphore
	
		sleep(rand()%5);
	}

}






