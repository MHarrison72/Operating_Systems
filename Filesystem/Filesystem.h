//Author: Michael Harrison
//Lab 4: Filesystem Structures

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//Size of...
#define FS_SIZE		0xa00000 //10MB filesystem
#define B_SIZE		512	 //block size in bytes
#define MF_SIZE		16384	 //max file size in bytes
#define NUM_INODES	512	 //number of inodes allocated during mounting
#define NUM_DIRECT_BLOCKS 14	

//Flag Definitions 
#define MFILE	 	0x00 //0000 0000 
#define DIRECTORY	0x01 //0000 0001
#define READ		0x02 //0000 0010
#define WRITE		0x04 //0000 0100
#define EXECUTE 	0x08 //0000 1000
#define	APPEND		0x10 //0001 0000
#define	DELETE		0x20 //0010 0000
#define	DIRTY		0x40 //0100 0000     
#define IN_USE		0x80 //1000 0000

#ifndef HEADER
#define HEADER
struct open_file{
	char name[32];
	unsigned long dir_num;
	unsigned long node_num;
	unsigned int count;
	unsigned int access_rights;
};


struct block{
	unsigned int block;
};

struct fBlock{
	struct block prev;
	struct block next;
};


struct address_space{
	struct block direct[NUM_DIRECT_BLOCKS];
	struct block indirect;
	struct block double_indirect; 
};

/*
.(current directory)
..(parent directory)
*/
struct directory{
	unsigned long i_no; //inode number
	char d_id[32]; //file name
};

struct inode{
	unsigned long i_num; 	//inode number
	unsigned int uid, gid;  //user id and group id for permission
	unsigned long i_size;	//size of file in bytes
	unsigned int i_flags; 		//Flags described above
	time_t creation;	//creation date
	time_t modified;	//modified date
	struct address_space i_data; //physical mapping 
	struct directory parent_dir;
};


struct superblock{
	char s_id[32]; //name of filesystem
	unsigned long s_block; //size of blocks 
	unsigned long s_inode; //size of inode
	unsigned long s_maxbytes; //max file size in bytes
	unsigned int s_free; //number of free blocks
	unsigned int s_used; //number of used blocks
	unsigned int s_flags; //Flags described above
	unsigned int s_first; //first free block
};

#endif

//API Functions
void next_free(void);
void create_fs(char *name);
void mount_fs(char *name);
void sync_fs(void);
void create(char *filename, char type);
void destroy(char *filename); 
void write(char *filename, char *message);
void read(char *filename);
void open(char *filename);
void close(char *filename);














