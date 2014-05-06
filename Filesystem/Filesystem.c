//Author: Michael Harrison
// Testing filesystem 

#include "Filesystem.h"

int main(int argc, char **argv){
	char *name = "filesystem_disk";
	create_fs(name);
	mount_fs(name);
	create("dir1", (READ | WRITE | DELETE | DIRECTORY) );
	open("dir1");
	create("file1", (READ | WRITE | DELETE | MFILE) );
	open("file1");
	write("file1", "THIS BETTER WORK!!!!\0");
	read("file1");
	close("file1");
	close("dir1");
	create("file3", (READ | WRITE | DELETE | MFILE) );
	//destroy("file 1");
	sync_fs();
	return 0;
}












