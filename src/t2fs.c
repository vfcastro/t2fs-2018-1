#include "../include/t2fs.h"
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0
#define ERROR -1

int T2FS_INIT = 0;
struct t2fs_superbloco SUPERBLOCK;

int init(){
	unsigned char buffer[SECTOR_SIZE];

	// Le o setor ZERO do disco
	if(read_sector(0,buffer) != SUCCESS){
    	printf("init(): read_sector() failed!\n");
    	return ERROR;
	}

	// Atualiza a global SUPERBLOCO a partir do setor ZERO lido
	memcpy(SUPERBLOCK.id,buffer,4);
	memcpy(&SUPERBLOCK.version,buffer+4,2);
	memcpy(&SUPERBLOCK.superblockSize,buffer+6,2);
	memcpy(&SUPERBLOCK.freeBlocksBitmapSize,buffer+8,2);
	memcpy(&SUPERBLOCK.freeInodeBitmapSize,buffer+10,2);
	memcpy(&SUPERBLOCK.inodeAreaSize,buffer+12,2);
	memcpy(&SUPERBLOCK.blockSize,buffer+14,2);
	memcpy(&SUPERBLOCK.diskSize,buffer+16,4);

	printf("%.4s\n",SUPERBLOCK.id);
	printf("%x\n",SUPERBLOCK.version);
	printf("%d\n",SUPERBLOCK.superblockSize);
	printf("%d\n",SUPERBLOCK.freeBlocksBitmapSize);
	printf("%d\n",SUPERBLOCK.freeInodeBitmapSize);
	printf("%d\n",SUPERBLOCK.inodeAreaSize);
	printf("%d\n",SUPERBLOCK.blockSize);
	printf("%d\n",SUPERBLOCK.diskSize);

	return SUCCESS;
}


int identify2 (char *name, int size){
	// Inicializa t2fs caso nao esteja
	if(!T2FS_INIT)
		if(init() == ERROR){
        	printf("identify2(): init() failed!\n");
	    	return ERROR;		
		}

	char id[] = "Vinicius Fraga de Castro 193026";
	if(strncpy(name,id,size) != NULL)
		return SUCCESS;
	else
		return ERROR; 
}


