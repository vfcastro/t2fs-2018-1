/*
	Testa a função cidentify2

	TESTE1: buffer valido, imprimir o nome do CWD
	TESTE2: buffer muito pequeno, deve retornar erro
*/


#include "../include/t2fs.h"
#include <stdio.h>

int main(){
	// TESTE1	
	char str1[200];

	if(getcwd2(str1,sizeof(str1)) == 0)
		printf("TESTE1: %s\n",str1);
	else
		printf("TESTE1: getcwd2(str1,sizeof(str1)) falhou!\n");


	// TESTE2
	char str2[1];

	if(getcwd2(str2,sizeof(str2)) == 0)
		printf("TESTE2: %s\n",str2);
	else
		printf("TESTE2: getcwd2(str2,sizeof(str2)) falhou!\n");
    

    return 0;
}
