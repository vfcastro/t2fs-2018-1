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
struct t2fs_inode INODE_ZERO;


int init(){
	unsigned char buffer[SECTOR_SIZE];

	// Le o setor ZERO do disco
	if(read_sector(0,buffer) != SUCCESS){
    	printf("init(): read_sector(0,buffer) failed!\n");
    	return ERROR;
	}
	// Atualiza a global SUPERBLOCO a partir do setor ZERO lido
	memcpy(&SUPERBLOCK,&buffer,sizeof(struct t2fs_superbloco));

	printf("%.4s\n",SUPERBLOCK.id);
	printf("%x\n",SUPERBLOCK.version);
	printf("%d\n",SUPERBLOCK.superblockSize);
	printf("%d\n",SUPERBLOCK.freeBlocksBitmapSize);
	printf("%d\n",SUPERBLOCK.freeInodeBitmapSize);
	printf("%d\n",SUPERBLOCK.inodeAreaSize);
	printf("%d\n",SUPERBLOCK.blockSize);
	printf("%d\n",SUPERBLOCK.diskSize); 


	// Calcula o setor do inode ZERO
	DWORD inode_zero_sector = SUPERBLOCK.blockSize * 
						(SUPERBLOCK.superblockSize+
						SUPERBLOCK.freeBlocksBitmapSize+
						SUPERBLOCK.freeInodeBitmapSize);
	// Le o inode ZERO do disco
	if(read_sector(inode_zero_sector,buffer) != SUCCESS){
    	printf("init(): read_sector(inode_zero_sector,buffer) failed!\n");
    	return ERROR;
	}
	// Atualiza a global INODE_ZERO com o inode lido
	memcpy(&INODE_ZERO,&buffer,sizeof(struct t2fs_inode));


	T2FS_INIT = 1;

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


