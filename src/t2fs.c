#include "../include/t2fs.h"
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

// 
int read_inode(DWORD inodeNumber, struct t2fs_inode *inode){
	unsigned char buffer[SECTOR_SIZE];

	// Calcula o setor do inode
	DWORD inode_sector = (SUPERBLOCK.blockSize * 
						(SUPERBLOCK.superblockSize+
						SUPERBLOCK.freeBlocksBitmapSize+
						SUPERBLOCK.freeInodeBitmapSize)) + inodeNumber*sizeof(struct t2fs_inode);
	// Le o setor do inode no disco
	if(read_sector(inode_sector,buffer) != SUCCESS){
    	printf("read_inode(): read_sector(inode_sector,buffer) failed!\n");
    	return ERROR;
	}
	// Copia o inode lido para o parametro *inode e retorna
	memcpy(inode,&buffer,sizeof(struct t2fs_inode));
	return SUCCESS;
}

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

// Aloca um HANDLE nas filas OPEN_DIRS ou OPEN_FILES e inicializa com record
int create_handle(FILA2 *fila, struct t2fs_record record){
	// ALOCA no na fila para o HANDLE
	PNODE2 nodeFila = (PNODE2)malloc(sizeof(NODE2));
	if(nodeFila == NULL){
		printf("create_handle(): (PNODE2)malloc(sizeof(NODE2)) failed!");
		return ERROR;
	}

	HANDLE *handle = (HANDLE*)malloc(sizeof(HANDLE));
	if(handle == NULL){
		printf("create_handle(): (HANDLE*)malloc(sizeof(HANDLE)) failed!");
		return ERROR;
	}

	nodeFila->node = handle;
	if(AppendFila2(fila,nodeFila) != SUCCESS){
		printf("create_handle(): AppendFila2(&fila=%d,nodeFila) failed!",(int)fila);
  		return ERROR;
	}

	handle->id = get_handle_id();
	handle->record.TypeVal = record.TypeVal;
	strcpy(handle->record.name,record.name);
	handle->record.inodeNumber = record.inodeNumber;	
	handle->current_offset = 0;

	print_handles(&OPEN_DIRS,"OPEN_DIRS");	
	return handle->id;
}

// Procura no diretorio dir pelo record de nome *name, e retorna o record
struct t2fs_record find_record_in_dir (char *name, struct t2fs_inode dir) {

	// Inicializa record de retorno e buffer de blocos
	struct t2fs_record record;
	strncpy(record.name,name,59);
	record.TypeVal = TYPEVAL_INVALIDO;
	unsigned char buffer[SECTOR_SIZE*SUPERBLOCK.blockSize];

	// Checa se o *name é invalido
	if(name == NULL || strlen(name) == 0){
		printf("find_record_in_dir(): name is NULL or empty!\n");
    	return record;		
	}
	
	// Percorre todos os blocos de dir
	int i,j;
	for(i=0; i < dir.blocksFileSize; i++){

		// Ponteiros DIRETOS (2)
		if(i < 2){

			// ERRO na leitura do bloco
			if(read_block(dir.dataPtr[i],buffer) == ERROR){
				printf("find_record_in_dir(): read_block(block=%u,&buffer=%d) failed!",dir.dataPtr[i],(int)buffer);
				return record;
			}
			
			// PROCURA record no bloco lido
			for(j=0; j < dir.bytesFileSize/sizeof(struct t2fs_record); j++){

				record.TypeVal = ((struct t2fs_record *)&buffer[j*sizeof(struct t2fs_record)])->TypeVal;
				strcpy(record.name,((struct t2fs_record *)&buffer[j*sizeof(struct t2fs_record)])->name);
				record.inodeNumber = ((struct t2fs_record *)&buffer[j*sizeof(struct t2fs_record)])->inodeNumber;
				//print_record( (struct t2fs_record *) &buffer[j*sizeof(struct t2fs_record)] );
				
				// ENCONTROU o record
				if( strcmp(record.name,name) == 0 && record.TypeVal != TYPEVAL_INVALIDO){
					//print_record(&record);
					return record;
				
				}
			}			
		}
		
		// Primeira INDIRECAO

		// Segunda INDIRECAO  
		
	}
	
	return record;
}

//
int get_handle(int id, PFILA2 origem, HANDLE *handle){
	// Seta o iterador no inicio da fila ORIGEM
	if(FirstFila2(origem) != SUCCESS){
    	//printf("check_tid(): FirstFila2(origem) failed!\n");
		return ERROR;
	}
	
	PNODE2 nodeFila;
	HANDLE *buffer;

	// Percorre a lista em busca pelo id 
	do{
		nodeFila = GetAtIteratorFila2(origem);
		if(nodeFila == NULL){
			printf("check_tid(): GetAtIteratorFila2(origem) failed!");
			return ERROR;
		}

		buffer = nodeFila->node;
		// Encontrou o tid, retorna sucesso
		if(buffer->id == id){
			memcpy(handle,buffer,sizeof(HANDLE));
			return SUCCESS;
		}

	}	
	while(NextFila2(origem) == SUCCESS);

	// Nao encontrou, retorna erro
	return ERROR;
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

	
	// Le o inodo ZERO e atualiza global
	if(read_inode(0,&INODE_ZERO) != SUCCESS){
    	printf("init(): read_inode(0,&INODE_ZERO) failed!\n");
    	return ERROR;
	}

	
	// Imprime dados do disco e structs
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
	printf("INODE_ZERO.dataPtr[1]: %d\n\n",INODE_ZERO.dataPtr[1]); 

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

	// Estruturas usadas para percorrer a arvore de diretorios
	struct t2fs_record record;
	struct t2fs_inode curr_inode;
	DIR2 handle;

	// Aloca string "path" para manipulacao via strtok
	char* token;
	char *path = (char*)malloc(strlen(pathname)+1);
	strcpy(path,pathname);
	token = strtok(path,DELIMITER);

	// Pathname eh o "/", abre o diretorio RAIZ e retorna o handle
	if(token == NULL) {

		// INICIALIZA record do "/"
		record.TypeVal = TYPEVAL_DIRETORIO;
		record.name[0] = '/';
		record.name[1] = '\0';
		record.inodeNumber = 0;	
		
		// Aloca e inicializa HANDLE para o diretorio RAIZ e retorna
		handle = create_handle(&OPEN_DIRS,record);
		if(handle == ERROR){
		   	printf("opendir2() failed to create handle for root dir \"/\"!\n");
			return ERROR;					
		}			

		return handle;
	}


	// Pathname ABSOLUTO:
	if(pathname[0] == '/')
		curr_inode = INODE_ZERO;
	else
		// Pathname RELATIVO:
		if(read_inode(CWD->record.inodeNumber,&curr_inode)){
		   	printf("opendir2() failed read inode of CWD!\n");
			return ERROR;					
		}		
		

	// Procura pelo record com o nome do token a partir do curr_inode
	do{
		record = find_record_in_dir(token,curr_inode);				
		printf("%s\n",token);
		token = strtok(NULL,DELIMITER);
		print_record(&record);
		
		// NAO ENCONTROU o record, libera recursos e retorna record INVALIDO
		if(record.TypeVal == TYPEVAL_INVALIDO){
			free(path);
			return ERROR;
		}

		// ENCONTROU no NIVEL, segue no caminhamento do path
		if(read_inode(record.inodeNumber,&curr_inode)){
		   	printf("opendir2() failed read inode of dir \"%s\"!\n",token);
			return ERROR;					
		}		

	}
	while(token != NULL);

	// ENCONTROU o diretorio, aloca e inicializa HANDLE para o diretorio e retorna
	handle = create_handle(&OPEN_DIRS,record);
	if(handle == ERROR){
	   	printf("opendir2() failed to create handle for root dir \"/\"!\n");
		return ERROR;					
	}			

	return handle;
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


FILE2 create2 (char *filename) {return ERROR;}
int delete2 (char *filename) {return ERROR;}
FILE2 open2 (char *filename) {return ERROR;}
int close2 (FILE2 handle) {return ERROR;}
int read2 (FILE2 handle, char *buffer, int size) {return ERROR;}
int write2 (FILE2 handle, char *buffer, int size) {return ERROR;}
int truncate2 (FILE2 handle) {return ERROR;}
int seek2 (FILE2 handle, DWORD offset) {return ERROR;}
int mkdir2 (char *pathname) {return ERROR;}
int rmdir2 (char *pathname) {return ERROR;}

int chdir2 (char *pathname) {
	// Inicializa t2fs caso nao esteja
	if(!T2FS_INIT)
		if(init() == ERROR){
        	printf("chdir2(): init() failed!\n");
	    	return ERROR;		
		}

	// DESALOCA o CWD


	// ABRE o arquivo pathname
	DIR2 handle = opendir2(pathname);
	if(handle != SUCCESS){
		printf("chdir2(): opendir(pathname) failed!\n");
	   	return ERROR;
	}

	// Atualiza CWD
	get_handle(handle,&OPEN_DIRS,CWD);



	return SUCCESS;
}

int readdir2 (DIR2 handle, DIRENT2 *dentry) {return ERROR;}
int closedir2 (DIR2 handle) {return ERROR;}

