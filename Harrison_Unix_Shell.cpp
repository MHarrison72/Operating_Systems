/* 
* Author Michael Harrison
* Unix command shell
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <iostream>

using namespace std;

//check argv for shell operands
void checkOper(char **argv, int &argc, int &opr, int &i, int &back); 
//remove elements from argv
void removeItems(char **argv, int start, int num); 
//execute two commands via a pipe
void exePipe(char **argv,int argc, int i); 
//execute a command with redirected input
void exeIn(char **argv, int argc, int i); 
//execute a command with redirected output
void exeOut(char **argv, int argc, int i); 
//execute a command in foreground or background
void exeCMD(char **argv, int back); 
//parse userInput into argv
void chop(char **argv, string s); 


int main(void){
	char *argv[100], dirName[100];
	string userInput;
	int i, argc, back, opr, thePipe[2];

	while(1){
		//reset important variables
		back = 0; i = 0; opr = 0;

		//get current directory
		getcwd(dirName,100);		

		//Print a prompt
		cout<<"Mike_Harrison:"<<string(dirName)<<"$ ";

		//Get the command line
		getline(cin,userInput);

		//check for exit conditions
		if ( userInput == "exit\0" || cin.eof()){
			cout<<endl;
			exit(0);
		}

		if(userInput == ""){printf("No input entered\n");}
		else{
			//Parse the command
			chop(argv, userInput);

			//Find number of elements in argv
			for(argc = 0; argv[argc] != NULL; argc++);

			//check for additional operators
			checkOper(argv, argc, opr, i, back);

			if(opr == 0){
				//Execute the command
				exeCMD(argv,back);
			}
			else if(opr == 1){
				//Execute the command with redirected input
				exeIn(argv,argc,i);
			}
			else if(opr == 2){
				//Execute the command with redirected output
				exeOut(argv,argc,i);
			}
			else if(opr == 3){
				//Execute the command with a pipe
				exePipe(argv,argc,i);
			}
			
		}
	}
}

/*
* Input: argument array argv 
*		argument counter argc, 
*		an int to determine operation to be performed, 
*		int i as a place holder, 
*		int back to determine if process should be run in background
* Assumptions: argument array will be scanned to check for I/O redirection, 
*		pipes, or putting a process in the background and will return assign 
*		a number to opr indicating which operation should be performed
*/
void checkOper(char **argv, int &argc, int &opr, int &i, int &back){
	for (int j = 1; j < argc; j++){
		char value = argv[j][0];			
		if(value == 38){ // &
			i = j;
			back = 1;
			removeItems(argv,i,1);
			argc -= 1;
			opr = 0;
		}
		else if(value == 60){ // <
			opr =  1;
			i = j;
		}
		else if(value == 62){ // >
			opr = 2;
			i = j;
		}
		else if(value == 124){ // |
			opr = 3;
			i = j;
		}
	}
}

/*
* Input: argument array 
*		element to first be removed, 
*		number of elements to remove
* Assumption: elements to be removed will be set to '\0'
*/
void removeItems(char **argv, int start, int num){
	for(int i = start; i < start+num; i++){
		argv[i] = '\0';
	}
}

/*
* Input: argument array argv, 
*		argument counter argc,
*		int i a placeholder
* Assumption: two child processes executing two commands with a pipe
*/
void exePipe(char **argv,int argc, int i){
	pid_t pid, pid2;
	int status, status2, thePipe[2];
	
	if (pid = fork() == 0) {  //Child1
		status2 = 0;
		removeItems(argv,i,1);
		argc -= 1;

		pipe(thePipe);
		pid2 = fork();
		
		//Child2
		if (pid2 > 0) {
			close(thePipe[1]);
			dup2(thePipe[0],0); // stdin to read portion
			close(thePipe[0]);
			execvp(argv[i+1],&argv[i+1]);
		}
		//Child1
		else if (pid2 == 0) {
			close(thePipe[0]);
			dup2(thePipe[1],1); //stdout to write portion
			close(thePipe[1]);
			execvp(argv[0],argv);
			wait(&status2);
		}
		
	}
	else{ //Parent
		status = 0;
		wait(&status);
		printf("Child exited with status of %d. \n", status);
	}
}


/*
* Input: argument array argv, 
*		argument counter argc, 
*		int i a placeholder
* Assumption: a child process executing the command with redirected input
*/
void exeIn(char **argv,int argc, int i){
	pid_t pid;
	int status, newIn;

	if (pid = fork() == 0) {  //Child
		newIn = open(argv[i+1], O_RDONLY);
		//perror("Error opening file");
		dup2(newIn,0);
		close(newIn);
		removeItems(argv,i,2);
		argc -= 2;
		execvp(argv[0],argv);
	}
	else{ //Parent
		status = 0;
		wait(&status);
		printf("Child exited with status of %d. \n", status);
	}
}

/*
* Input: argument array argv, 
*		argument counter argc, 
*		int i as a placeholder
* Assumption: a child process executing the command with redirected output
*/
void exeOut(char **argv, int argc, int i){
	pid_t pid;
	int status, newOut;

	if (pid = fork() == 0) {  //Child
		newOut = open(argv[i+1], O_WRONLY|O_CREAT,S_IRWXG|S_IRWXO|S_IROTH);
		//perror("Error opening file");
		dup2(newOut,1);
		close(newOut);
		removeItems(argv,i,2);
		argc -= 2;
		execvp(argv[0],argv);
	}
	else{ //Parent
		status = 0;
		wait(&status);
		printf("Child exited with status of %d. \n", status);
	}
}

/*
* Input: argument array 
*		int detemining if process should be run in background
* Assumption: a child process executing the command
*/
void exeCMD(char **argv, int back){
	pid_t pid;
	int status;

	if (back == 0) {
		if (pid = fork() == 0) {  //Child
			execvp(argv[0],argv);
		}
		else{ //Parent
			status = 0;
			wait(&status);
			printf("Child exited with status of %d. \n", status);
		}
	}
	else if (back == 1) {
		if (pid = fork() == 0) {  //Child
			execvp(argv[0],argv);
		}
		else { //Parent
			printf("Child process put in background.\n");
		}
	}
}

/*
* Input: s = string to be broken apart
*        c = delimeter at which s is split
* Output: a NULL terminated array of null-terminated strings
* Assumptions: breaks apart string s at every occasion of char c
*		and places each new string into a dynamically allocated array
*/
void chop(char **argv, string s){
	int i = 0;

	while (s[i] != '\0') {       // if not the end of line
		while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n'){
			s[i] = '\0';     // replace white spaces with 0
			i++;
		}

		*argv++ = &s[i];          // save the argument position

		while (s[i] != '\0' && s[i] != ' ' && s[i] != '\t' && s[i] != '\n'){
		       i++;             // skip the argument until ...
		}

		if(s[i] == '"'){
			i++;
			while(s[i] != '"'){i++;}
		}
	}
	*argv = '\0';                 // mark the end of argument list
}

