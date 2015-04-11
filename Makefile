#
# Makefile for Sudoku solver
#
CXX = g++
FLAGS =  -g -Wall -Wall -Wextra -pedantic -O3

all: sudoku.cpp
	${CXX} ${FLAGS} -o sudoku sudoku.cpp; rm -f core.*; rm -f vgcore*

clean:
	rm -f sudoku;rm -f core.*; rm -f vgcore*

