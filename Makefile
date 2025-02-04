C-FLAGS=-Wall -Wextra -Wpedantic -O3
SRC=./src
DST=./bin

all: build build-helper

build: | bin
	gcc $(C-FLAGS) $(SRC)/terminal.c $(SRC)/main.c -o $(DST)/hecto

build-helper: | bin 
	gcc $(C-FLAGS) $(SRC)/terminal.c $(SRC)/helper.c -o $(DST)/helper

bin:
	mkdir ./bin
