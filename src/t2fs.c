#include "../include/t2fs.h"
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0
#define ERROR -1

int T2FS_INIT = 0;

int init(){
	return SUCCESS;
}


int identify2 (char *name, int size){
	// Inicializa cthread caso nao esteja
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


