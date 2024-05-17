
OUT=./mvm
SRC=src/*.c ../mutils/*.c
INC=-I src/ -I ../mutils/

all:
	tcc -o $(OUT) $(SRC) $(INC)

masm: all
	tcc -o masm masm.c ../mutils/*.c  -I ../mutils/ -I ./src/
