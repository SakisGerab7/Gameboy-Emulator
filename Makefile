CC = g++
CFLAGS = -Wall -g -fsanitize=address
LIBS = -lm -lpthread -lSDL2
EXECNAME = gbemu
SRCDIR = src
OBJDIR = obj
BINDIR = bin
SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
BIN = $(BINDIR)/$(EXECNAME)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -r $(OBJ) $(BIN)