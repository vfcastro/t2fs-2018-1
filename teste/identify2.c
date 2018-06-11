/* 
	Testa a função cidentify2
*/

#include "../include/t2fs.h"
#include <stdio.h>

int main(){
	
	char str[200];

	identify2(str,sizeof(str));
	printf("%s\n",str);
    

    return 0;
}
