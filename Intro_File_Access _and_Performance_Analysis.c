/*
* Author:Michael Harrison
* Intro File Access and Performance Analysis
*/

#include <stdio.h>
#include <stdlib.h>

//Variables placed into struct for organization & ease of access 
struct Record {
	char result_time[18];
	char result_blocks[64][20];
	int line_status;
	float line_data, relativetime;
	unsigned long blkhdr_ticks;
};

void readRecord(struct Record *records, FILE* fp);
int readAgain();

/*
* Array of structs used as data structure
* Do-While loop used to control reading next block
* For loop used to fill each struct in array
* NRecords is number of records to be read
* start is the number record to start at
*/
int main(int argc, char **argv){
	int i, NRecords = 0, start = 0, size_of_record = 17+19*64;
	char *filename = "input.txt";
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL){
		printf("Could not open file.\n");
		return 0;
	}
	struct Record *records;

	do {
		//determine number of records to be read
		NRecords = rand()%4999+1;	
		//determine number record to start at
		start = rand()%4999+1;
		//place file pointer at starting point
		fseek(fp, start * size_of_record, SEEK_SET);

		//allocate memory for records (1298 characters per record)
		records = malloc(sizeof(*records) * NRecords);

		//loop to read in data
		for(i = 0; i < NRecords; i++){
			readRecord(&records[i], fp); 
		}

		//delay three seconds
		sleep(3000);

		//deallocate memory
		free(records);	

		//return file pointer to beginning of file
		rewind(fp);

	} while(readAgain());

	printf("Goodbye.\n");
	return 0;
}

/*
* input: single struct from array and pointer to input file
* output: data into variables from individual struct in array
*/
void readRecord(struct Record *records, FILE* fp){
	int i;
	//to read the 1st line -- different than succeeding lines
	fread(&records->result_time, 18, 1, fp);

	//to decode 1st line into data elements	
	sscanf(records->result_time, "%d%f", &records->blkhdr_ticks, &records->relativetime);

	//fetch all 64 lines (lines 2-65) of data at once
	fread(&records->result_blocks, 20, 64, fp);

	//loop to decode each line
	for(i = 0; i < 65; i++){
		sscanf(records->result_blocks[i], "%d%f", &records->line_status, &records->line_data); //status<SPACE>data
	}
}

/*
* prompts user to read another set of records
* output: 1 if continue and 0 if exit
*/
int readAgain(){
	char c;
	printf("Read again? (y/n):\n");
	scanf("%c",&c);
	if (c == 'y'){return 1;}
	else{return 0;}
}
















