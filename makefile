LIB_DIR=./lib/
INC_DIR=./include/
BIN_DIR=./bin/
SRC_DIR=./src/

all: t2fs.o libt2fs.a

t2fs.o: 
	gcc -c -o $(BIN_DIR)t2fs.o $(SRC_DIR)t2fs.c -Wall
libt2fs.a:
	ar crs $(LIB_DIR)libt2fs.a $(BIN_DIR)t2fs.o $(LIB_DIR)apidisk.o $(LIB_DIR)bitmap2.o
clean:
	rm -rf $(LIB_DIR)libt2fs.a $(BIN_DIR)t2fs.o
