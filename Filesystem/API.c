//Author: Michael Harrison
//Lab 4: Filesystem API

#include "Filesystem.h"

static FILE *disk;
static struct superblock sb;
static char i_bmap[B_SIZE];
static struct inode pool[NUM_INODES];
//could be linked list to top and bottom directories
static struct directory dir_table[NUM_INODES][NUM_INODES];
/* ^^


	0			1		3		4		5
name	i_num		name	i_num					name	i_num
.	0		.	1					.	5
..	NULL		..	0					..	1
home	1		file	2					file2	3
			dir	5

*/
static unsigned int current_dir;
static struct open_file open_table[NUM_INODES];

//advance superblock pointer to next free block
void next_free(void){
	sb.s_free--; sb.s_used++; sb.s_flags = (sb.s_flags | DIRTY);
	struct fBlock info;
	fseek(disk, sb.s_first*B_SIZE, SEEK_SET);
	fread(&info.prev.block, sizeof(info.prev.block), 1, disk); 	//should always be -1(NULL)
	fread(&info.next.block, sizeof(info.next.block), 1, disk);	//next free block
	sb.s_first = info.next.block;				//set superblock to first free block
	fseek(disk, info.next.block*B_SIZE, SEEK_SET);	//seek to first free block
	fwrite(&info.prev.block, sizeof(info.prev.block), 1, disk);	//set it to -1(NULL)
	return;
}

void create_fs(char *name){
	int i;
	struct superblock super;
	struct inode nodes[NUM_INODES];
	struct inode root_node, empty;
	struct directory root_dir;

	//create filesystem partition 
	FILE *fp = fopen(name,"wb");

	//extend filesystem to desired size
	fseek(fp, FS_SIZE, SEEK_SET);
	fputc('0', fp);
	fclose(fp);

	fp = fopen(name,"r+b");

	//initalize superblock
	strcpy(super.s_id,"Michael_Harrison_Filesystem");
	super.s_block = B_SIZE;
	super.s_inode = sizeof(struct inode);
	super.s_maxbytes = MF_SIZE;
	super.s_free = (FS_SIZE/B_SIZE) - 2 - NUM_INODES/(B_SIZE/sizeof(struct inode));
	super.s_used = 2 + NUM_INODES/(B_SIZE/sizeof(struct inode));
	super.s_flags = (IN_USE | READ);
	super.s_first = super.s_used + 1;
	

	//create root directory inode
	root_node.i_num = 0;
	root_node.uid = 0x1234;
	root_node.gid = 0x1234;
	root_node.i_size = sizeof(struct directory)*2;
	root_node.i_flags = (DIRECTORY | READ | WRITE | IN_USE);
	time(&root_node.creation);
	time(&root_node.modified);
	root_node.i_data.direct[0].block = super.s_first;
	root_node.parent_dir.i_no = -1;
	strcpy(root_node.parent_dir.d_id, "\0");

	super.s_first++;
	super.s_used++;
	super.s_free--;

	//create zeroed out inode to ensure no excess data
	empty.i_num = 0;
	empty.uid = 0;
	empty.gid = 0;
	empty.i_size = 0;
	empty.i_flags = 0;
	empty.creation = 0;
	empty.modified = 0;
	for(i = 0; i < NUM_DIRECT_BLOCKS; i++){empty.i_data.direct[i].block = 0;}
	empty.i_data.indirect.block = 0;
	empty.i_data.double_indirect.block = 0;
	empty.parent_dir.i_no = 0;
	strcpy(empty.parent_dir.d_id, "");

	//set 1st inode in pool to root node
	nodes[0] = root_node;
	//set remainder to empty
	for(i = 1; i < NUM_INODES; i++){nodes[i] = empty;}

	//create root directory dir
	root_dir.i_no = root_node.i_num;
	strcpy(root_dir.d_id,".");

	//write superblock to disk
	fwrite(&super.s_id, sizeof(super.s_id), 1, fp);
	fwrite(&super.s_block, sizeof(super.s_block), 1, fp);
	fwrite(&super.s_inode, sizeof(super.s_inode), 1, fp);
	fwrite(&super.s_maxbytes, sizeof(super.s_maxbytes), 1, fp);
	fwrite(&super.s_free, sizeof(super.s_free), 1, fp);
	fwrite(&super.s_used, sizeof(super.s_used), 1, fp);
	fwrite(&super.s_flags, sizeof(super.s_flags), 1, fp);
	fwrite(&super.s_first, sizeof(super.s_first), 1, fp);
	fflush(fp);
	printf("Superblock written\n");
	
	//write inode bitmap to disk
	unsigned char zero[B_SIZE];
	zero[0] = 0x80;
	for(i = 1; i < B_SIZE; i++){
		zero[i] = 0x00;
	}
	fseek(fp, B_SIZE, SEEK_SET);
	fwrite(zero, sizeof(unsigned char), B_SIZE, fp);
	fflush(fp);

	printf("Inode bitmap written\n");

	//write inode pool to disk
	for(i = 0; i < NUM_INODES; i++){
		fwrite(&nodes[i].i_num, sizeof(nodes[i].i_num), 1, fp);
		fwrite(&nodes[i].uid, sizeof(nodes[i].uid), 1, fp);
		fwrite(&nodes[i].gid, sizeof(nodes[i].gid), 1, fp);
		fwrite(&nodes[i].i_size, sizeof(nodes[i].i_size), 1, fp);
		fwrite(&nodes[i].i_flags, sizeof(nodes[i].i_flags), 1, fp);
		fwrite(&nodes[i].creation, sizeof(nodes[i].creation), 1, fp);
		fwrite(&nodes[i].modified, sizeof(nodes[i].modified), 1, fp);
		fwrite(&nodes[i].i_data.direct[0].block, sizeof(nodes[i].i_data.direct[0].block), 1, fp);
		fwrite(&nodes[i].parent_dir.i_no, sizeof(nodes[i].parent_dir.i_no), 1, fp);
		fwrite(&nodes[i].parent_dir.d_id, sizeof(nodes[i].parent_dir.d_id), 1, fp);
		fflush(fp);
	}

	printf("Inode pool written\n");

	//write struct directory root_dir to disk
	fwrite(&root_dir.i_no, sizeof(root_dir.i_no), 1, fp);
	fwrite(&root_dir.d_id, sizeof(root_dir.d_id), 1, fp);
	fflush(fp);
	root_dir.i_no = -1;
	strcpy(root_dir.d_id,"..");
	fwrite(&root_dir.i_no, sizeof(root_dir.i_no), 1, fp);
	fwrite(&root_dir.d_id, sizeof(root_dir.d_id), 1, fp);
	fflush(fp);
	root_dir.i_no = -1;
	strcpy(root_dir.d_id,"\0");
	fwrite(&root_dir.i_no, sizeof(root_dir.i_no), 1, fp);
	fwrite(&root_dir.d_id, sizeof(root_dir.d_id), 1, fp);
	fflush(fp);

	
	//set free block pointers
	int num_blocks = super.s_free;
	struct fBlock info;
	info.prev.block = -1;
	info.next.block = super.s_first+1;
	fseek(fp, super.s_first*B_SIZE, SEEK_SET);
	for(i = 0; i < num_blocks-1; i++){
		fwrite(&info.prev.block, sizeof(info.prev.block), 1, fp);
		fwrite(&info.next.block, sizeof(info.next.block), 1, fp);
		fflush(fp);
		fseek(fp, B_SIZE-sizeof(info), SEEK_CUR);
		info.prev.block = info.next.block-1; info.next.block++;
	}
	info.next.block = -1;
	fwrite(&info.prev.block, sizeof(info.prev.block), 1, fp);
	fwrite(&info.next.block, sizeof(info.next.block), 1, fp);
	fflush(fp);

	printf("Free pointers written\n");
	

	printf("\nName of filesystem: |%s| was created\n", super.s_id);
	
	fclose(fp);
}

//bring filesystem into memory
void mount_fs(char *name){
	disk = fopen(name,"r+b");
	int i, j;

	//bring superblock into memory
	fread(&sb, sizeof(sb), 1, disk);

	//bring inode bitmap into memory
	fseek(disk, B_SIZE - sizeof(struct superblock), SEEK_CUR);
	fread(&i_bmap, 1, B_SIZE, disk);

	//bring inode pool into memory
	for(i = 0; i < NUM_INODES; i++){
		fread(&pool[i].i_num, sizeof(pool[i].i_num), 1, disk);
		fread(&pool[i].uid, sizeof(pool[i].uid), 1, disk);
		fread(&pool[i].gid, sizeof(pool[i].gid), 1, disk);
		fread(&pool[i].i_size, sizeof(pool[i].i_size), 1, disk);
		fread(&pool[i].i_flags, sizeof(pool[i].i_flags), 1, disk);
		fread(&pool[i].creation, sizeof(pool[i].creation), 1, disk);
		fread(&pool[i].modified, sizeof(pool[i].modified), 1, disk);
		fread(&pool[i].i_data.direct[0].block, sizeof(pool[i].i_data.direct[0].block), 1, disk);
		fread(&pool[i].parent_dir.i_no, sizeof(pool[i].parent_dir.i_no), 1, disk);
		fread(&pool[i].parent_dir.d_id, sizeof(pool[i].parent_dir.d_id), 1, disk);
	}

	//bring directory table into memory
	for(i = 0; i < NUM_INODES; i++){
		if( pool[i].i_flags&DIRECTORY == DIRECTORY){
			j = 0;
			fread(&dir_table[i][j].i_no, sizeof(dir_table[i][j].i_no), 1, disk);
			fread(&dir_table[i][j].d_id, sizeof(dir_table[i][j].d_id), 1, disk);
			while( strcmp(dir_table[i][j].d_id, "\0") ){
				j++;
				fread(&dir_table[i][j].i_no, sizeof(dir_table[i][j].i_no), 1, disk);
				fread(&dir_table[i][j].d_id, sizeof(dir_table[i][j].d_id), 1, disk);
			}
		}
	}
	
	//set current directory to root
	current_dir = 0;

	//initalize open file table
	struct open_file empty;
	strcpy(empty.name, "");
	empty.dir_num = -1;
	empty.node_num = -1;
	empty.count = -1;
	empty.access_rights = -1;
	for(i = 0; i < NUM_INODES; i++){
		open_table[i] = empty;
	}

	//set root directory in open file table
	strcpy(open_table[0].name, "/");
	open_table[0].dir_num = -1;
	open_table[0].node_num = 0;
	open_table[0].count = 1;
	empty.access_rights = pool[0].i_flags;

	printf("\nFilesystem |%s| successfully mounted.\n", sb.s_id);

	return;
}

//write everything back to disk
void sync_fs(void){
	int i, j;
	rewind(disk);
	
	//write superblock to disk
	if( (sb.s_flags & DIRTY) == DIRTY){
		sb.s_flags = (sb.s_flags & ~DIRTY);
		fwrite(&sb.s_id, sizeof(sb.s_id), 1, disk);
		fwrite(&sb.s_block, sizeof(sb.s_block), 1, disk);
		fwrite(&sb.s_inode, sizeof(sb.s_inode), 1, disk);
		fwrite(&sb.s_maxbytes, sizeof(sb.s_maxbytes), 1, disk);
		fwrite(&sb.s_free, sizeof(sb.s_free), 1, disk);
		fwrite(&sb.s_used, sizeof(sb.s_used), 1, disk);
		fwrite(&sb.s_flags, sizeof(sb.s_flags), 1, disk);
		fwrite(&sb.s_first, sizeof(sb.s_first), 1, disk);
		fflush(disk);
	}
	
	
	//write inode bitmap to disk
	fseek(disk, B_SIZE, SEEK_SET);
	fwrite(&i_bmap, sizeof(char), B_SIZE, disk);
	fflush(disk);

	//write inode pool to disk
	for(i = 0; i < NUM_INODES; i++){
		//if( (pool[i].i_flags & DIRTY) == DIRTY){
			pool[i].i_flags = (pool[i].i_flags & ~DIRTY);
			//fseek(disk, 2*B_SIZE+sizeof(struct inode)*(i+1), SEEK_SET);
			fwrite(&pool[i].i_num, sizeof(pool[i].i_num), 1, disk);
			fwrite(&pool[i].uid, sizeof(pool[i].uid), 1, disk);
			fwrite(&pool[i].gid, sizeof(pool[i].gid), 1, disk);
			fwrite(&pool[i].i_size, sizeof(pool[i].i_size), 1, disk);
			fwrite(&pool[i].i_flags, sizeof(pool[i].i_flags), 1, disk);
			fwrite(&pool[i].creation, sizeof(pool[i].creation), 1, disk);
			fwrite(&pool[i].modified, sizeof(pool[i].modified), 1, disk);
			fwrite(&pool[i].i_data.direct[0].block, sizeof(pool[i].i_data.direct[0].block), 1, disk);
			fwrite(&pool[i].parent_dir.i_no, sizeof(pool[i].parent_dir.i_no), 1, disk);
			fwrite(&pool[i].parent_dir.d_id, sizeof(pool[i].parent_dir.d_id), 1, disk);
			fflush(disk);
		//}
	}

	//write directory information to disk
	for(i = 0; i < NUM_INODES; i++){
		if( (pool[i].i_flags & DIRECTORY) == DIRECTORY){
			j = 0;
			//fseek(disk, pool[i].i_data.direct[0].block*B_SIZE, SEEK_SET);
			do{
				fwrite(&dir_table[i][j].i_no, sizeof(dir_table[i][j].i_no), 1, disk);
				fwrite(&dir_table[i][j].d_id, sizeof(dir_table[i][j].d_id), 1, disk);
				fflush(disk); j++;
			}while( strcmp(dir_table[i][j].d_id, "\0") );
			fwrite(&dir_table[i][j].i_no, sizeof(dir_table[i][j].i_no), 1, disk);
			fwrite(&dir_table[i][j].d_id, sizeof(dir_table[i][j].d_id), 1, disk);
			fflush(disk);
		}
	}

	printf("\nDisk synced\n");
	
	fclose(disk);
}

/*
	allocate inode and single block for data
*/

void create(char *filename, char type){
	int i, j, k;
	char c = IN_USE;
	int x = IN_USE;
	
	//find free inode: int i will determine position in bitmap
	for(i = 0; (i < NUM_INODES) && ((c&x) == x); i++){
		c = i_bmap[i]; x = 0x01; x = x<<7;
		for(j = 0; j < 8 && ((c&x) == x); j++){
			x = x>>1;
		}
	}

	if(i == NUM_INODES && (c&x) == x){
		printf("\nNo free inodes\n");
		return;
	}

	//check for free block
	unsigned int free_block = sb.s_first;	//pointer to first free block
	if (free_block == 0){
		printf("\nNo free blocks\n");
		return;
	}

	//mark in inode bitmap
	i_bmap[i-1] = ( i_bmap[i-1] | x);

	//fill in inode data values
	struct inode inode;
	inode.i_num = j; 				//inode number
	inode.uid = 0x1234;  				//user id
	inode.gid = 0x1234;  				//group id
	inode.i_size = 0;				//size of file in bytes
	inode.i_flags = type | DIRTY | IN_USE; 		//Flags described above
	time(&inode.creation);				//creation date
	time(&inode.modified);				//modified date
	inode.i_data.direct[0].block = free_block; 	//physical mapping 
	inode.parent_dir.i_no = current_dir;
	//determine parent dir name
	i = dir_table[current_dir][1].i_no;
	if (i == -1){
		strcpy(inode.parent_dir.d_id, "/");
	}
	else{
		for(k = 0; k < NUM_INODES && strcmp(dir_table[i][k].d_id, "\0") && dir_table[i][k].i_no != current_dir; k++){;}
		strcpy(inode.parent_dir.d_id, dir_table[i][k].d_id);
	}

	//set superblock free pointer to next block
	next_free();
	

	//store inode in pool
	pool[j] = inode;

	//create directory entry
	for(k = 0; (k < NUM_INODES) && strcmp(dir_table[current_dir][k].d_id, "\0"); k++){;}
	dir_table[current_dir][k].i_no = j;
	strcpy(dir_table[current_dir][k].d_id, filename);
	dir_table[current_dir][k+1].i_no = -1;
	strcpy(dir_table[current_dir][k+1].d_id, "\0");
	pool[current_dir].i_size += sizeof(struct directory);

	if ( (type & DIRECTORY) == DIRECTORY){
		//create directory in table
		dir_table[j][0].i_no = inode.i_num;
		strcpy(dir_table[j][0].d_id, ".");
		dir_table[j][1].i_no = current_dir;
		strcpy(dir_table[j][1].d_id, "..");
		dir_table[j][2].i_no = -1;
		strcpy(dir_table[j][2].d_id, "\0");
		pool[j].i_size += sizeof(struct directory)*2;
	}


	printf("\nFile created\n");

	return;
}

/*
	write data to file blocks on disk

*/ 
void write(char *filename, char *message){
	int i, j;
	//determine number of blocks needed 
	int block_num = ceil((double)strlen(message)/(double)B_SIZE);
	//determine if num blocks free
	if (block_num > 1 && sb.s_free < block_num){
		printf("\nInsufficient free blocks.\n");
		return;
	}

	//locate file
	for(i = 0; i < NUM_INODES && strcmp(dir_table[current_dir][i].d_id, "\0") && strcmp(dir_table[current_dir][i].d_id, filename); i++);
	unsigned long node_num = dir_table[current_dir][i].i_no;

	//check access rights
	struct inode inode = pool[node_num];
	if( (inode.i_flags & WRITE) != WRITE ){
		printf("\nYou do not have permission to write to this file.\n");
		return;
	}

	//allocate blocks to inode
	unsigned int indirect_blocks[block_num - NUM_DIRECT_BLOCKS]; //array of block numbers
	if(block_num != (inode.i_size/B_SIZE) ){
		sb.s_free -= block_num; sb.s_used += block_num;
		struct fBlock info;
		for(i = 0; i < NUM_DIRECT_BLOCKS && i < block_num; i++){
			inode.i_data.direct[i].block = sb.s_first;
			next_free();
		}
		if(block_num > NUM_DIRECT_BLOCKS){
			inode.i_data.indirect.block = sb.s_first;
			next_free();
			for(i = 0; i < block_num - NUM_DIRECT_BLOCKS; i++){
				indirect_blocks[i] = sb.s_first;
				next_free();
			}
			//write indirect block to disk
			fseek(disk, inode.i_data.indirect.block*B_SIZE, SEEK_SET);
			fwrite(&indirect_blocks, sizeof(indirect_blocks), 1, disk);
			fflush(disk);
		}
	}

	//break data into B_SIZE chunks for writing
	char bstr[block_num][B_SIZE];
	for(i = 0; i < block_num; i++){
		strncpy(bstr[i], &message[i*B_SIZE], B_SIZE);
	}

	//write bstr onto disk
	for(i = 0; i < block_num; i++){
		for(; i < NUM_DIRECT_BLOCKS && i < block_num; i++){
			fseek(disk, inode.i_data.direct[i].block*B_SIZE, SEEK_SET);
			fwrite(&bstr[i], B_SIZE, 1, disk);
			fflush(disk);
		}
		for(j = 0; j < (block_num - NUM_DIRECT_BLOCKS); j++, i++){
			fseek(disk, indirect_blocks[j]*B_SIZE, SEEK_SET);
			fwrite(&bstr[j], B_SIZE, 1, disk);
			fflush(disk);
		}
	}

	//update modified date and dirty and size
	time(&inode.modified);
	inode.i_flags = (inode.i_flags | DIRTY);
	inode.i_size = strlen(message);
	
	//update inode in pool
	pool[node_num] = inode;

	printf("\nWrite successful\n");

	return;
}

/*
	prints data from file to screen
*/
void read(char *filename){
	int i, j;
	//locate file
	for(i = 0; (i < NUM_INODES) && strcmp(dir_table[current_dir][i].d_id, "\0") && strcmp(dir_table[current_dir][i].d_id, filename); i++);
	if( !strcmp(dir_table[current_dir][i].d_id, "\0") ){
		printf("\nFile not in current directory.\n"); 
		return;
	}
	unsigned long node_num = dir_table[current_dir][i].i_no;

	//check access rights
	struct inode inode = pool[node_num];
	if( (inode.i_flags & READ) != READ ){
		printf("\nYou do not have permission to read this file.\n");
		return;
	}

	//determine number of blocks to read
	int block_num = ceil((double)inode.i_size/(double)B_SIZE);


	//read bstr from disk
	char bstr[block_num][B_SIZE];
	for(i = 0; i < block_num; i++){
		for(; i < NUM_DIRECT_BLOCKS && i < block_num; i++){
			fseek(disk, inode.i_data.direct[i].block*B_SIZE, SEEK_SET);
			fread(&bstr[i], B_SIZE, 1, disk);
			fflush(disk);
		}
		if(block_num > NUM_DIRECT_BLOCKS){	
			unsigned int indirect_blocks[block_num - NUM_DIRECT_BLOCKS]; //array of block numbers
			fseek(disk, inode.i_data.indirect.block*B_SIZE, SEEK_SET);
			fread(&indirect_blocks, sizeof(indirect_blocks), 1, disk);
			for(j = 0; j < (block_num - NUM_DIRECT_BLOCKS); j++, i++){
				fseek(disk, indirect_blocks[j]*B_SIZE, SEEK_SET);
				fread(&bstr[j], B_SIZE, 1, disk);
			}
		}
	}

	//print data to screen
	for(i = 0; i < block_num; i++){
		printf("\n%s\n", bstr[i]);
	}

	printf("\nFinished reading file.\n");
	return;
}

/*
delete inode data and dellocate blocks back to free list
*/
void destroy(char *filename){
	int i, j;
	//locate file
	for(i = 0; (i < NUM_INODES) && strcmp(dir_table[current_dir][i].d_id, "\0") && strcmp(dir_table[current_dir][i].d_id, filename); i++);
	unsigned long node_num = dir_table[current_dir][i-1].i_no;

	//check access rights
	struct inode inode = pool[node_num];
	if( (inode.i_flags & DELETE) != DELETE){
		printf("\nYou do not have permission to delete this file.\n");
		return;
	}

	//determine number of blocks
	int block_num = ceil((double)inode.i_size/(double)B_SIZE);

	//deallocate blocks to inode
	sb.s_free += block_num; sb.s_used -= block_num;
	struct fBlock info;
	info.prev.block = inode.i_data.direct[0].block; info.next.block = sb.s_first;
	fseek(disk, sb.s_first*B_SIZE, SEEK_SET);
	fwrite(&info.prev.block, sizeof(info.prev.block), 1, disk);
	sb.s_first = inode.i_data.direct[0].block;
	for(i = 1; i < NUM_DIRECT_BLOCKS && i < block_num; i++){
		fseek(disk, info.prev.block*B_SIZE, SEEK_SET);
		info.next.block = info.prev.block;
		info.prev.block = inode.i_data.direct[i].block;
		fwrite(&info.prev.block, sizeof(info.prev.block), 1, disk);
		fwrite(&info.next.block, sizeof(info.next.block), 1, disk);
		sb.s_first = inode.i_data.direct[i].block;
		fflush(disk);
	}

	if(block_num > NUM_DIRECT_BLOCKS){
		unsigned int indirect_blocks[block_num - NUM_DIRECT_BLOCKS]; //array of block numbers
		fseek(disk, inode.i_data.indirect.block*B_SIZE, SEEK_SET);
		fread(&indirect_blocks, sizeof(indirect_blocks), 1, disk);
		info.next.block = info.prev.block;
		info.prev.block = inode.i_data.indirect.block;
		fwrite(&info.prev.block, sizeof(info.prev.block), 1, disk);
		fwrite(&info.next.block, sizeof(info.next.block), 1, disk);
		fflush(disk);
		for(j = 0; j < (block_num - NUM_DIRECT_BLOCKS); j++){
			fseek(disk, indirect_blocks[j]*B_SIZE, SEEK_SET);
			info.next.block = info.prev.block;
			info.prev.block = indirect_blocks[j];
			fwrite(&info.prev.block, sizeof(info.prev.block), 1, disk);
			fwrite(&info.next.block, sizeof(info.next.block), 1, disk);
			fflush(disk);
		}
		sb.s_first = indirect_blocks[j];	
	}

	//free spot in inode bitmap
	int map = (node_num/8), shift = (node_num % 8);
	char x = 0x01; x = x<<(7-shift); x = ~x;	//1000 0000 => 1110 1111
	i_bmap[map] = (i_bmap[map] & x);          //1001 1101 => 1000 1101

	//clear inode in pool and mark dirty
	struct inode empty;
	empty.i_num = 0;
	empty.uid = 0;
	empty.gid = 0;
	empty.i_size = 0;
	empty.i_flags = DIRTY;
	empty.creation = 0;
	empty.modified = 0;
	for(i = 0; i < NUM_DIRECT_BLOCKS; i++){empty.i_data.direct[i].block = 0;}
	empty.i_data.indirect.block = 0;
	empty.i_data.double_indirect.block = 0;
	empty.parent_dir.i_no = 0;
	strcpy(empty.parent_dir.d_id, "");
	pool[node_num] = empty;

	//clear directory entry
	for(j = 0; (j < NUM_INODES) && strcmp(dir_table[current_dir][j].d_id, "\0"); j++){;}
	dir_table[current_dir][j-1].i_no = -1;
	strcpy(dir_table[current_dir][j-1].d_id, "");

	printf("\nFile destroyed.\n");

	return;
}

/*
create/increment file in open file table
*/

void open(char *filename){
	int i, j = 0, k;
	struct open_file entry;
	//see if file is already opened
	for(k = 0; (k < NUM_INODES) && strcmp(open_table[k].name, "\0") && strcmp(open_table[k].name, filename); k++){;}
	if(strcmp(open_table[k].name, filename)){
		//locate file (i = dir #) (j = # in dir)
		for(i = 0; (i < NUM_INODES) && strcmp(dir_table[i][j].d_id, "\0") && strcmp(dir_table[i][j].d_id, filename); i++){
			for(j = 0; (j < NUM_INODES) && strcmp(dir_table[i][j].d_id, "\0") 
					&& strcmp(dir_table[i][j].d_id, filename); j++);
		}
		strcpy(entry.name, filename);
		entry.dir_num = i-1;
		entry.node_num = dir_table[i-1][j].i_no;
		entry.count = 1;
		entry.access_rights = pool[entry.node_num].i_flags;

		open_table[k] = entry;

		if(entry.access_rights&DIRECTORY == DIRECTORY){current_dir = dir_table[i-1][j].i_no;}
		else{	
		
		}			
	}
	else{open_table[k].count++;}

	return;
}


//close file in open file table
void close(char *filename){
	int i;
	struct open_file entry;
	//see if file is already opened
	for(i = 0; (i < NUM_INODES) && strcmp(open_table[i].name, "\0") && strcmp(open_table[i].name, filename); i++){;}
	if(!strcmp(open_table[i].name, filename)){
		if( open_table[i].access_rights&DIRECTORY == DIRECTORY && open_table[i].dir_num != (unsigned long)(-1)){
			current_dir = pool[open_table[i].node_num].parent_dir.i_no;
		}
		if(open_table[i].count > 1){
			open_table[i].count--;
		}
		else{
			open_table[i] = open_table[i+1];
			i++;
			while( strcmp(open_table[i].name, "\0")){
				open_table[i] = open_table[i+1];
				i++;
			}
		}
	}
	else{printf("\nFile did not exist in open file table.\n");}

	return;
}











