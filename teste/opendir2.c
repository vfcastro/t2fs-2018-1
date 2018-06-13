/*
	Testa a função opendir2

	TESTE1: pathname vazio, retorna erro.
	TESTE2: pathname "/", abre diretorio raiz e retorna handle
	TESTE3: pathname "/dir1/", abre diretorio "dir1" e retorna sucesso
	TESTE4: pathname ".././../.././dir2", abre diretorio e retorna sucesso

*/


#include "../include/t2fs.h"
#include <stdio.h>

int main(){
	char *path;
	int handle;

	// TESTE1
	path = "";
	handle = opendir2(path);
	if(handle >= 0)
		printf("TESTE1: opendir2(\"%s\"), handle: %d\n",path,handle);
	else
		printf("TESTE1: opendir2(\"%s\") falhou!\n",path);

	// TESTE2
	path = "/";
	handle = opendir2(path);
	if(handle >= 0)
		printf("TESTE2: opendir2(\"%s\"), handle: %d\n",path,handle);
	else
		printf("TESTE2: opendir2(\"%s\") falhou!\n",path);
    
	// TESTE3
	path = "/dir1";
	handle = opendir2(path);
	if(handle >= 0)
		printf("TESTE3: opendir2(\"%s\"), handle: %d\n",path,handle);
	else
		printf("TESTE3: opendir2(\"%s\") falhou!\n",path);

	// TESTE4
	path = ".././../.././dir2";
	handle = opendir2(path);
	if(handle >= 0)
		printf("TESTE4: opendir2(\"%s\"), handle: %d\n",path,handle);
	else
		printf("TESTE4: opendir2(\"%s\") falhou!\n",path);

	
	path = "/dir2";
	chdir2(path);


    return 0;
}
