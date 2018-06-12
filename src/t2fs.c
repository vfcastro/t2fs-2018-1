#include "../include/t2fs.h"
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h> 


#define SUCCESS 0
#define ERROR -1
#define DELIMITER "/"

// Struct do handle de um arquivo/diretorio aberto
typedef struct {
	int id; // identificador
	struct t2fs_record record; // entrada de diretorio
	DWORD current_offset; // deslocamento em bytes (current entry ou current pointer)
} HANDLE;

int T2FS_INIT = 0;
int HANDLE_ID = 0;
FILA2 OPEN_DIRS;
FILA2 OPEN_FILES;
HANDLE* CWD;
struct t2fs_superbloco SUPERBLOCK;
struct t2fs_inode INODE_ZERO;

// Imprime um t2fs_record
void print_record (struct t2fs_record *record){
	printf("TypeVal:%u\n",record->TypeVal);
	printf("name:%s\n",record->name);
	printf("inodeNumber:%u\n",record->inodeNumber);
}

// Le um bloco do disco e copia para a area de memoria *buffer
int read_block (DWORD block, unsigned char *buffer){
	// Checa se o bloco eh maior que o tamanho do disco
	if(block > SUPERBLOCK.diskSize-1){
		printf("read_block(): block:%d bigger then disk size: %d!\n",block,SUPERBLOCK.diskSize-1);
    	return ERROR;		
	}	

	// Checa se o buffer é invalido
	if(buffer == NULL){
		printf("read_block(): buffer is NULL!\n");
    	return ERROR;		
	}

	// Calcula o setor inicial do bloco no disco
	DWORD sector = block * SUPERBLOCK.blockSize;

	// Le os setores do disco e grava no buffer
	int i;
	for(i=0; i<SUPERBLOCK.blockSize; i++){
		if(read_sector(sector,buffer) != SUCCESS){
			printf("read_block(): read_sector(sector=%d,buffer=%d) failed!\n",sector,(int)buffer);
			return ERROR;
		}
		sector = sector + 1;
		buffer = buffer + SECTOR_SIZE;
	}
	
	// Realizou a leitura com sucesso
	return SUCCESS;
}

// Imprime HANDLEs abertos nas filas
void print_handles(PFILA2 fila, char* name){
	printf("print_handles(&%s):\n",name);
	PNODE2 nodeFila;
	HANDLE *handle;
	if(FirstFila2(fila) != SUCCESS)
    	printf("print_handles(&%s): FirstFila2(fila) failed!\n",name);
	else {
		do {
			nodeFila = GetAtIteratorFila2(fila);
			handle = nodeFila->node;
			printf("id:%d\nrecord.TypeVal:%x\nrecord.name:\"%s\"\nrecord.inodeNumber:%d\nhandle->current_offset:%d\n",
					handle->id,
					handle->record.TypeVal,
					handle->record.name,
					handle->record.inodeNumber,
					handle->current_offset);
		}
		while(NextFila2(fila) == SUCCESS);
	}
}

// Retorna TID sequencial
int get_handle_id(){
	int id = HANDLE_ID;
	HANDLE_ID = HANDLE_ID + 1;
	return id;
}

// Inicializacao T2FS
int init(){
	unsigned char buffer[SECTOR_SIZE];

	// Le o setor ZERO do disco
	if(read_sector(0,buffer) != SUCCESS){
    	printf("init(): read_sector(0,buffer) failed!\n");
    	return ERROR;
	}
	// Atualiza a global SUPERBLOCO a partir do setor ZERO lido
	memcpy(&SUPERBLOCK,&buffer,sizeof(struct t2fs_superbloco));

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

	
	/* Imprime dados do disco e structs
	printf("SECTOR_SIZE: %d\n\n",SECTOR_SIZE);
	printf("SUPERBLOCK.id: %.4s\n",SUPERBLOCK.id);
	printf("SUPERBLOCK.version: %x\n",SUPERBLOCK.version);
	printf("SUPERBLOCK.superblockSize: %d\n",SUPERBLOCK.superblockSize);
	printf("SUPERBLOCK.freeBlocksBitmapSize: %d\n",SUPERBLOCK.freeBlocksBitmapSize);
	printf("SUPERBLOCK.freeInodeBitmapSize: %d\n",SUPERBLOCK.freeInodeBitmapSize);
	printf("SUPERBLOCK.inodeAreaSize: %d\n",SUPERBLOCK.inodeAreaSize);
	printf("SUPERBLOCK.blockSize: %d\n",SUPERBLOCK.blockSize);
	printf("SUPERBLOCK.diskSize: %d\n\n",SUPERBLOCK.diskSize); 
	printf("sizeof(struct t2fs_record): %d\n",sizeof(struct t2fs_record));
	printf("sizeof(struct t2fs_inode): %d\n\n",sizeof(struct t2fs_inode));
	printf("INODE_ZERO.blocksFileSize: %d\n",INODE_ZERO.blocksFileSize);  
	printf("INODE_ZERO.bytesFileSize: %d\n",INODE_ZERO.bytesFileSize);
	printf("INODE_ZERO.dataPtr[0]: %d\n",INODE_ZERO.dataPtr[0]);
	printf("INODE_ZERO.dataPtr[1]: %d\n\n",INODE_ZERO.dataPtr[1]); */

	// Inicializa filas de arquivos e diretorios abertos
	if(CreateFila2(&OPEN_DIRS) != SUCCESS || CreateFila2(&OPEN_FILES) != SUCCESS){
    	printf("init(): CreateFila2(&OPEN_DIRS) || CreateFila2(&OPEN_FILES) failed!");
		return ERROR;
	}

	// Aloca e inicializa HANDLE para o diretorio RAIZ
    PNODE2 nodeFila = (PNODE2)malloc(sizeof(NODE2));
    if(nodeFila == NULL){
		printf("init(): (PNODE2)malloc(sizeof(NODE2)) failed!");
    	return ERROR;
	}

	HANDLE *root_handle = (HANDLE*)malloc(sizeof(HANDLE));
    if(root_handle == NULL){
		printf("init(): (HANDLE*)malloc(sizeof(HANDLE)) failed!");
    	return ERROR;
	}

	nodeFila->node = root_handle;
	if(AppendFila2(&OPEN_DIRS,nodeFila) != SUCCESS){
		printf("init(): AppendFila2(&OPEN_DIRS,nodeFila) failed!");
  		return ERROR;
	}

	root_handle->id = get_handle_id();
	root_handle->record.TypeVal = TYPEVAL_DIRETORIO;
	root_handle->record.name[0] = '/';
	root_handle->record.name[1] = '\0';
	root_handle->record.inodeNumber = 0;	
	root_handle->current_offset = 0;

	// Inicializa o CWD (current work dir) com o handle do diretorio RAIZ
	CWD = root_handle;
	
	// Fim da inicializacao da T2FS
	T2FS_INIT = 1;

	//print_handles(&OPEN_DIRS,"OPEN_DIRS");

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

DIR2 opendir2 (char *pathname){
	// Inicializa t2fs caso nao esteja
	if(!T2FS_INIT)
		if(init() == ERROR){
        	printf("opendir2(): init() failed!\n");
	    	return ERROR;		
		}

	// Checa se o pathname de entrada eh invalido
	if(pathname == NULL) {
       	printf("opendir2() failed! pathname is NULL!\n");
    	return ERROR;		
	}
	if(strlen(pathname) < 1) {
       	printf("opendir2() failed! pathname is empty!\n");
    	return ERROR;		
	}

	// Se o pathname for ABSOLUTO:
	char* token;
	if(pathname[0] == '/'){
		
		// Aloca string "path" para manipulacao via strtok
		char *path = (char*)malloc(strlen(pathname)+1);
		strcpy(path,pathname);
		token = strtok(path,DELIMITER);

		// Se o path eh o "/", abre o diretorio RAIZ e retorna o handle
		if(token == NULL) {
			// Aloca e inicializa HANDLE para o diretorio RAIZ
			PNODE2 nodeFila = (PNODE2)malloc(sizeof(NODE2));
			if(nodeFila == NULL){
				printf("opendir2(): (PNODE2)malloc(sizeof(NODE2)) failed!");
				return ERROR;
			}

			HANDLE *root_handle = (HANDLE*)malloc(sizeof(HANDLE));
			if(root_handle == NULL){
				printf("opendir2(): (HANDLE*)malloc(sizeof(HANDLE)) failed!");
				return ERROR;
			}

			nodeFila->node = root_handle;
			if(AppendFila2(&OPEN_DIRS,nodeFila) != SUCCESS){
				printf("opendir2(): AppendFila2(&OPEN_DIRS,nodeFila) failed!");
		  		return ERROR;
			}

			root_handle->id = get_handle_id();
			root_handle->record.TypeVal = TYPEVAL_DIRETORIO;
			root_handle->record.name[0] = '/';
			root_handle->record.name[1] = '\0';
			root_handle->record.inodeNumber = 0;	
			root_handle->current_offset = 0;
			
			print_handles(&OPEN_DIRS,"OPEN_DIRS");
			return root_handle->id;
		}

		// Pathname contem pelo menos um nivel a partir do diretorio raiz
		else {
		// Procura pelo arquivo com o nome do token encontrado no diretorio raiz 

/*			unsigned char buffer[SECTOR_SIZE*SUPERBLOCK.blockSize];
			if(read_block(67,buffer) == ERROR){
				printf("read_block(67,buffer) == ERROR");
				return ERROR;
			}
			print_record((struct t2fs_record*)buffer);  */

			do{
				printf("%s\n",token);
				token = strtok(NULL,DELIMITER);
			}
			while(token != NULL);


		}


	}

	// Se o pathname for RELATIVO:

		



	return SUCCESS;
}

int getcwd2 (char *pathname, int size){
	// Inicializa t2fs caso nao esteja
	if(!T2FS_INIT)
		if(init() == ERROR){
        	printf("getcwd2(): init() failed!\n");
	    	return ERROR;		
		}

	// Checa se o CWD é válido
	if(CWD == NULL){
       	printf("getcwd2(): CWD is NULL!\n");
    	return ERROR;	
	}

	// Checa se o tamanho do buffer *pathname eh menor que o nome do CWD
	// CWD->record.name + 1 por causa do terminador de string
	if(strlen(CWD->record.name)+1 > size){
       	printf("getcwd2(): *pathname size is too small! CWD size is %d bytes, but %d bytes buffer given.\n",strlen(CWD->record.name)+1,size);
    	return ERROR;
	}

	// Copia o nome de CWD para o buffer *pathname
	if(strncpy(pathname,CWD->record.name,strlen(CWD->record.name)+1) != NULL)
		return SUCCESS;
	else{
       	printf("getcwd2(): strncpy failed!\n");
    	return ERROR;	
	}
}
