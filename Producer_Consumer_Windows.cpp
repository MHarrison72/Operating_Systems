/*
* Author: Michael Harrison
* Multiple Producer-Consumer Problem : Windows
*/

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <windows.h>

using namespace std;

#define N 100
#define numT 6

int buff[N], front, rear;
HANDLE logFile;
HANDLE fullSem, emptySem;
HANDLE depositLock, fetchLock;
HANDLE producers[numT], consumers[numT];

DWORD WINAPI prodRandInt(LPVOID lpParam);
DWORD WINAPI consRandInt(LPVOID lpParam);

int main(void){
	int i = 0;
	front = 0; rear = 0;
	
	//clear buffer
	for(i = 0; i < N; i++){
		buff[i] = 0;
	}

	//initalize mutexes and semaphores
	depositLock = CreateMutex(NULL, FALSE, NULL);
	fetchLock = CreateMutex(NULL, FALSE, NULL);
	fullSem = CreateSemaphore(NULL, 0, N, NULL);
	emptySem = CreateSemaphore(NULL, N, N, NULL);

	//create threads
	for(i = 0; i < numT; i++){
		producers[i] = CreateThread(NULL, 0, prodRandInt, NULL, NULL, NULL);
		printf("Created Producer thread #%d\n", i);
		consumers[i] = CreateThread(NULL, 0, consRandInt, NULL, NULL, NULL);
		printf("Created Consumer thread #%d\n", i);
	}

	//wait for threads to finish
	WaitForMultipleObjects(numT, producers, TRUE, INFINITE);
	WaitForMultipleObjects(numT, consumers, TRUE, INFINITE);	

	//destroy threads
	for( i=0; i < numT; i++ ){
		CloseHandle(producers[i]);
		CloseHandle(consumers[i]);
	}

	//destroy mutexes and semaphores
	CloseHandle(depositLock);
	CloseHandle(fetchLock);
	CloseHandle(fullSem);
	CloseHandle(emptySem);

	return 0;
}

/*
* Output: printing a string of thread ID, number deposited, and position deposited to
* Assumptions: Will check empty semaphore to determine if buffer has free slot, then try and obtain mutex to deposit.
*        Deposits data in buffer at position rear and increments rear to next open slot.
*	     Gives up mutex for deposit, then increments full semaphore indicating successful deposit
*/
DWORD WINAPI prodRandInt(LPVOID lpParam){
	int depVal;

	while(1){
		depVal = rand(); //create data to be deposited
		
		WaitForSingleObject(emptySem, 0L); //empty slot?

		WaitForSingleObject(depositLock,INFINITE); //mutual exculsion for consume		

        	buff[rear] = depVal; //deposit data
		rear = (rear % (N-1)) + 1;
		printf("Producer #%lu deposited %d & rear = %d\n", GetCurrentThreadId(), depVal, rear);
                	    		
		ReleaseMutex(depositLock); 
		
		ReleaseSemaphore(fullSem, 1, NULL); //increase full semaphore

		Sleep(rand()%5);
	}

	return 0;
}

/*
* Output: printing a string of thread ID, number fetched, and position fetched from
* Assumptions: Will check full semaphore to determine if buffer has data, then try and obtain mutex to fetch.
*        Fetches data from buffer at position front and increments front to next filled slot.
*	     Gives up mutex for fetch, then increments empty semaphore indicating successful fetch
*/
DWORD WINAPI consRandInt(LPVOID lpParam){
	int fetVal;

	while(1){		
		WaitForSingleObject(fullSem, 0L); //filled slot?

		WaitForSingleObject(fetchLock,INFINITE); //mutual exculsion for consume		
	
        	fetVal = buff[front]; //fetch data to be consumed
		front = (front % (N-1)) + 1;
		printf("Consumer #%lu fetched %d & front = %d\n", GetCurrentThreadId(), fetVal, front);	
                	    		
        	ReleaseMutex(fetchLock);

		ReleaseSemaphore(emptySem, 1, NULL); //increase empty semaphore
	
		Sleep(rand()%5);
	}

	return 0;
}






