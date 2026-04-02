CC = gcc
LEX = flex
YACC = bison
YFLAGS = -d -y -Wcounterexamples
CFLAGS = -Wall -Wno-unused-function -g

TARGET = jucompiler
OBJS = y.tab.o lex.yy.o ast.o

SCRIPT = ../comp/test.sh

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lfl

y.tab.c y.tab.h: jucompiler.y
	$(YACC) $(YFLAGS) jucompiler.y

lex.yy.c: jucompiler.l y.tab.h
	$(LEX) jucompiler.l

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

clean:
	rm -f $(TARGET) y.tab.c y.tab.h lex.yy.c *.o
test: all
	./$(SCRIPT) ./$(TARGET)


.PHONY: all clean
