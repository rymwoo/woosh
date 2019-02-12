woosh: history.o lex.yy.o woosh.o
	g++ -o woosh history.o lex.yy.o woosh.o

history.o: history.cpp history.h
	g++ -c history.cpp

lex.yy.o: lex.yy.c
	g++ -c lex.yy.c

woosh.o: woosh.cpp woosh.h
	g++ -c woosh.cpp

lex.yy.c: parse.l parse.h
	lex parse.l

clean:
	rm woosh history.o lex.yy.o woosh.o

debug: history.o lex.yy.o woosh.o
	g++ -o woosh history.o lex.yy.o woosh.o -g