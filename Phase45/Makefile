all: scanner.c parser.c calc.out clean

calc.out: *.c *.h vm/*.c vm/exec_func/*.c
	gcc -g -Ivm -o calc *.c vm/*.c vm/exec_func/*.c -lm
scanner.c: scanner.l
	flex -outfile=scanner.c scanner.l

parser.c: parser.y
	bison --defines --output=parser.c parser.y

clean:
	rm -rf a.out parser.c scanner.c parser.h  output_lex.txt output_yacc.txt quads.txt
