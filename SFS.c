#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct inode_t{
	int size;
	int num_of_blocks;
	int block_pointer;
	char type;
} inode_t;

typedef struct dir_t{
	char *filenames[100];
	int filesizes[100];
	int fileinodes[100];
	int num_of_files;
} dir_t;

//disk size is 256KB
const int block_size =4096;
const int inode_blocks=5;
const int data_blocks = 56;
const int inode_start_address= 12288;
const int data_start_block = 8;
const int inode_bitmap_block = 1;
const int data_bitmap_block = 2;
const int inode_size= 256;
const int root_inode = 2;

char INODE_BITMAP[4096];			
char DATA_BITMAP[4096];

int currid;
FILE *disk[10];
char *dnames[10];

//FILE *disk;


void open_all(){
	for (int i = 0; i < currid; ++i){
		disk[i]=fopen(dnames[i],"r+b");
	}
}

void close_all(){
	for (int i = 0; i < currid; ++i){
		fclose(disk[i]);
	}
}

int readData(int fsid,int blockNum,void *block){
//	printf("readData called at address %d\n",block_size*blockNum);
	fseek(disk[fsid],block_size*blockNum,SEEK_SET);
	int i=0;
	fread(block,1,block_size,disk[fsid]);
    return 0;
}

int writeData(int fsid,int blockNum,void *block){
//	printf("writeData called at address %d\n",block_size*blockNum);
	int t = fseek(disk[fsid],block_size*blockNum,SEEK_SET);
	//printf("in write seek returned: %d\n",t );
	fwrite(block,1,block_size,disk[fsid]);
	return 0;
}

inode_t* create_inode(char type,int size){
//	printf("create_inode called\n");
	inode_t *new = (inode_t*)malloc(sizeof(inode_t));
	new->size=size;
	new->num_of_blocks = size/block_size + ((size%block_size)!=0);
	new->type = type;
	return new;
}

int write_inode(int fsid,inode_t *ptr,int inum){
	//printf("write_inode called at address %d\n",inode_start_address + (inum*inode_size));
	char *temp = (char*)malloc(sizeof(char)*inode_size);
	temp = (char*)ptr;
	int t = inode_start_address + (inum*inode_size);
	t = fseek(disk[fsid],t,SEEK_SET);
	//printf("in inodeWrite seek returned: %d\n",t );
	if(t!=0) return 1;
	fwrite(temp,1,inode_size,disk[fsid]);
	return 0;
}

char* read_inode(int fsid,int inum){
	//printf("read_inode called at address %d\n",inode_start_address + (inum*inode_size));
	int t=inode_start_address+(inum*inode_size);
	t=fseek(disk[fsid],t,SEEK_SET);
	if(t!=0) return NULL;
	char *temp = (char*)malloc(sizeof(char)*inode_size);
	fread(temp,1,inode_size,disk[fsid]);
	return temp;
}


int give_dspace(int inum){
	//printf("give_dspace called\n");
	int i=0;
	while(i < 56){
		if(DATA_BITMAP[i]=='0'){
			DATA_BITMAP[i]='1';
			return i;
			//ptrs[count++]=i;
		}
		i++;
	}
	INODE_BITMAP[inum] = '0';
	return -1;
}

int give_inum(){
	//printf("give_inum called\n");
	//readData(inode_bitmap_block,INODE_BITMAP);
	for (int i = 3; i < 80; ++i){
		if(INODE_BITMAP[i]=='0'){
			INODE_BITMAP[i]='1';
			return i;
		}
	}
	return -1;
}

dir_t* create_dir(){
	//printf("create_dir called\n");
	dir_t *new = (dir_t*)malloc(sizeof(dir_t));
	new->num_of_files =0;
	//new->filenames = (char**)malloc(sizeof(char*)*100);
	//new->filesizes =(int*)malloc(sizeof(int)*100);
	//new->fileinodes = (int*)malloc(sizeof(int)*100);
	return new;
}

int writeFile(int fsid,char *filename,void *block){
	//printf("writeFile called\n");	
	inode_t *i = (inode_t*)read_inode(fsid,root_inode);
	char data[block_size];
	readData(fsid,data_start_block+i->block_pointer,data);
	dir_t* rootdir = (dir_t*)data;
	char **this = rootdir->filenames;
	int j;
	for(j=0; j<rootdir->num_of_files; j++)
	{
		if(!strcmp(this[j],filename)){
			printf("\"%s\" file already exists!! Please choose a different name.\n", this[j] );
			return 1;
		}
	}

	readData(fsid,inode_bitmap_block,INODE_BITMAP);
	readData(fsid,data_bitmap_block,DATA_BITMAP);
	char *arr = (char*) block;
	int id = give_inum();
	if(id==-1){
		printf("Cannot write more files! Out of inodes\n");
		return 1;
	}
	inode_t *ptr = create_inode('f',strlen(arr));
	//printf("File size: %d\n",ptr->size );


	//ptr->block_pointers = (int*)malloc(sizeof(int)*ptr->num_of_blocks);
	ptr->block_pointer = give_dspace(id);//,ptr->size,ptr->block_pointers);
	if(ptr->block_pointer==-1){
		printf("Cannot write file! Insufficient disk Space\n");
		return 1;
	}

	//printf("write inode: %d\n",id );
	//printf("write block: %d\n",data_start_block+ptr->block_pointer);
	writeData(fsid,data_start_block+ptr->block_pointer,block);//!=0);// return 1;
	write_inode(fsid,ptr,id);

	inode_t *root = (inode_t*)read_inode(fsid,root_inode);
	char rootbuff[block_size];
	readData(fsid,data_start_block+root->block_pointer,rootbuff);
	rootdir =(dir_t*)rootbuff;
	rootdir->filenames[rootdir->num_of_files] = filename;
	rootdir->filesizes[rootdir->num_of_files] = ptr->size;
	rootdir->fileinodes[rootdir->num_of_files] = id;
	rootdir->num_of_files++;
	root->size += ptr->size;
	writeData(fsid,data_start_block+root->block_pointer,(char*)rootdir);
	write_inode(fsid,root,root_inode);
	writeData(fsid,inode_bitmap_block,INODE_BITMAP);
	writeData(fsid,data_bitmap_block,DATA_BITMAP);
	return 0;
}

int readFile(int fsid,char *filename,void *block){
	//printf("readFile called\n");
	inode_t *root = (inode_t*)read_inode(fsid,root_inode);
	char rootbuff[block_size];
	readData(fsid,data_start_block+root->block_pointer,rootbuff);
	dir_t *rootdir =(dir_t*)rootbuff;
	for (int i = 0; i < rootdir->num_of_files; ++i){
		if(!strcmp(rootdir->filenames[i],filename)){
			inode_t *ptr=(inode_t*)read_inode(fsid,rootdir->fileinodes[i]);
			//printf("read inode: %d\n", rootdir->fileinodes[i]);
			//printf("read block: %d\n",data_start_block+ ptr->block_pointer);
			readData(fsid,data_start_block+ptr->block_pointer,block);
			return 0;
		}
	}
	printf("Error reading file: \"%s\" file does not exist in specified file system\n",filename);
	return 1;
}

void createSFS(char *filename){
	printf("createSFS called\n");
	for (int i = 0; i < currid; ++i){
		if(!strcmp(filename,dnames[i])){
			printf("The file \"%s\" is already a disk!!\n", filename);
			return;
		}
	}
	dnames[currid] = filename;
	disk[currid] = fopen(filename,"wb+");
	for (int i = 0; i < 56; ++i){
		DATA_BITMAP[i]='0';
	}
	for (int i = 0; i < 80; ++i){
		INODE_BITMAP[i]='0';
	}
	INODE_BITMAP[2]='1';
	
	dir_t *rootdir = create_dir();
	inode_t *root = create_inode('d',sizeof(rootdir));
	root->block_pointer = give_dspace(root_inode);
	if(root->block_pointer==-1){
		printf("Error initializing disk %s\n",filename);
		return;
	}
	writeData(currid,data_start_block+root->block_pointer,(char*)rootdir);
	write_inode(currid,root,root_inode);
	writeData(currid,inode_bitmap_block,INODE_BITMAP);
	writeData(currid,data_bitmap_block,DATA_BITMAP);
	printf("\"%s\" file initialized as file system with fsid %d\n",filename,currid );
	currid++;
}


void printFileList(int fsid){
	inode_t *i = (inode_t*)read_inode(fsid,root_inode);
	char data[block_size];
	readData(fsid,data_start_block+i->block_pointer,data);
	dir_t* rootdir = (dir_t*)data;
	char **this = rootdir->filenames;
	int j;
	for(j=0; j<rootdir->num_of_files; j++)
	{
		printf("%s\n", this[j] );
	}
}


void printInodeBitmap(int fsid){
	int x=0;
	char imap[4096];
	readData(fsid,inode_bitmap_block,imap);
	for(x=0; x<80; x++)
	{
		printf("%c ", imap[x] );
	}
	printf("\n");
}

void printDataBitmap(int fsid){
	char dmap[4096];
	readData(fsid,data_bitmap_block,dmap);
	int x=0;
	for(x=0; x<data_blocks; x++)
	{
		printf("%c ", dmap[x] );
	}
	printf("\n");
}


int main(){
	currid=0;
	char buff1[block_size]="this is test 1 data for file 1";
	char buff2[block_size] = "this is test 2 data for file 1";
	char ans1[block_size],ans2[block_size];
	char ptr[100] = "file";
	createSFS("file");
	for (char i = 49; i < 122; ++i){
		strcat(ptr, &i);
		if(writeFile(0,ptr,buff1)) break;
	}
	createSFS("file2");
	writeFile(0,"file1t1",buff1);
	strcpy(buff1,"this is test 1 data for file 2");
	writeFile(0,"file2t1",buff1);
	writeFile(1,"file1t2",buff2);
	strcpy(buff2,"this is test 2 data for file 2");
	writeFile(1,"file2t2",buff2);
	printf("Data map for 1:\n"); printDataBitmap(0);
	printf("Inode map for 2:\n"); printInodeBitmap(1);
	printf("File list for 1:\n"); printFileList(0);
	printf("File list for 2:\n"); printFileList(1);
	printf("Data in file1t1:\n");
	readFile(0,"file1t1",ans1);
	printf("%s\n",ans1);
	printf("Data in file2t2:\n");
	readFile(1,"file2t3",ans2);
	printf("%s\n",ans2 );
	close_all();
	return 0;
}
