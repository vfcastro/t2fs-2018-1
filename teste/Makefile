LIB_DIR=../lib/
BIN_DIR=../exemplo/

all: identify2 getcwd2 opendir2
 
identify2:
	gcc -o -L$(BIN_DIR)identify2 identify2.c -L$(LIB_DIR) -lt2fs -Wall

getcwd2:
	gcc -o -L$(BIN_DIR)getcwd2 getcwd2.c -L$(LIB_DIR) -lt2fs -Wall

opendir2:
	gcc -o -L$(BIN_DIR)opendir2 opendir2.c -L$(LIB_DIR) -lt2fs -Wall

clean:
	rm -rf -L$(BIN_DIR)identify2 -L$(BIN_DIR)getcwd2 -L$(BIN_DIR)opendir2
