CXX=g++
LD=g++
CXXFLAGS=-Wall -pedantic -Wno-long-long -O0 -ggdb --std=c++14 -I src/include
LIBS=-lncurses -lform

all: spreadsheet

spreadsheet: src/main.o \
	src/Sheet.o \
	src/Address.o \
	src/Type.o \
	src/CellBase.o \
	src/formula/Parser.o \
	src/formula/function/Add.o \
	src/formula/function/Sub.o \
	src/formula/function/Mul.o \
	src/formula/function/Div.o \
	src/formula/function/Abs.o \
	src/formula/function/Sin.o \
	src/formula/function/Cos.o \
	src/formula/function/Tan.o \
	src/Utils.o \
	src/UI.o

	$(LD) $(LIBS) -o spreadsheet $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# todo: .h dependencies

run:
	./spreadsheet

clean:
	rm -rf spreadsheet \
		src/main.o \
		src/Sheet.o \
		src/Address.o \
		src/Type.o \
		src/CellBase.o \
		src/formula/Parser.o \
		src/formula/function/Add.o \
		src/formula/function/Sub.o \
		src/formula/function/Mul.o \
		src/formula/function/Div.o \
		src/formula/function/Abs.o \
		src/formula/function/Sin.o \
		src/formula/function/Cos.o \
		src/formula/function/Tan.o \
		src/Utils.o \
		src/UI.o \
		doc
