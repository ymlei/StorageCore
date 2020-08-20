CC_RISCV = riscv64-unknown-elf-g++
CC_ARM = arm-linux-gnueabihf-g++

SRC = tpch_query1_gem5.cpp util.c
#OBJ = $(patsubst %.c, %.o, $(SRC))
OBJ = tpch_query1_gem5.o util.o

#CFLAGS = ""

arm : $(SRC)
	$(CC_ARM) -static -o $@ $^ $(CFLAGS)

riscv : $(SRC)
	$(CC_RISCV) -o $@ $^ $(CFLAGS)