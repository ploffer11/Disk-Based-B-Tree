.SUFFIXES: .cc .o

CXX=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cc
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB+=$(wildcard src/*.cc)
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cc=.o)

CXXFLAGS+= -pthread -std=c++17 -g -fPIC -I $(INC)

TARGET=main

all: $(TARGET)

$(TARGET): $(OBJS_FOR_LIB)
	make static_library
	$(CXX) $(CXXFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

library:
	$(CXX) -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
