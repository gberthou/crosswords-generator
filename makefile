GPP=g++
BIN=crosswords

OBJDIR=obj

LIBDIR= 
INCDIR=-I"."

CFLAGS=-std=c++17 -Wall -Wextra -Werror -pedantic -O2 -g

DEFINES=
LIBS=-lpthread -lgecodekernel -lgecodesearch -lgecodeint -lgecodeminimodel -lgecodesupport -lgecodedriver -lgecodegist

CPPFILES=$(wildcard *.cpp)

OBJS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(CPPFILES))

$(OBJDIR)/%.o : %.cpp
	$(GPP) $(CFLAGS) $(INCDIR) -c $< -o $@ $(DEFINES)

default: $(BIN)

$(BIN): $(OBJS)
	$(GPP) $(OBJS) -o $(BIN) $(LIBDIR) $(LIBS)

build:
	mkdir -p $(OBJDIR)

clean:
	rm -f $(BIN) $(OBJS)

.PHONY: check
check:
	cppcheck -I. --inconclusive --enable=all .
