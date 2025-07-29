all: analyzer.out

analyzer.out: alpha_yylex.c
	gcc -o analyzer alpha_yylex.c

alpha_yylex.c:
	flex -outfile=alpha_yylex.c lexical_analyzer.l

clean:
	rm -rf alpha_yylex.c a.out

clean_txt:
	rm -rf *.txt
