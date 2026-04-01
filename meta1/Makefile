CC = gcc
LEX = flex
TARGET = jucompiler
LEX_FILE = jucompiler.l
GEN_C = lex.yy.c
FILE = code.txt
SCRIPT = comp/test.sh

all: $(TARGET)

$(TARGET): $(GEN_C)
	$(CC) $(GEN_C) -o $(TARGET) -lfl

$(GEN_C): $(LEX_FILE)
	$(LEX) $(LEX_FILE)

clean:
	rm -f $(TARGET) $(GEN_C)

test: all
	./$(SCRIPT) ./$(TARGET)
