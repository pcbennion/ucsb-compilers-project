#
# makefile for Project 1, part 1
#

YACC    = bison -d -v
LEX     = flex
CC      = gcc
CPP     = g++ -std=c++11 -g
ASTBUILD = ./astbuilder.gawk

TARGET	= lang

OBJS += lexer.o parser.o main.o ast.o primitive.o  ast2dot.o symtab.o classhierarchy.o typecheck.o codegen.o
RMFILES = core.* lexer.cpp parser.cpp parser.hpp parser.output ast.hpp ast.cpp $(TARGET) $(OBJS) start

# dependencies
$(TARGET): parser.cpp lexer.cpp parser.hpp $(OBJS)
	$(CPP) -o $(TARGET) $(OBJS)

# rules
%.cpp: %.ypp
	$(YACC) -o $(@:%.o=%.d) $<

%.o: %.cpp
	$(CPP) -o $@ -c $<

%.cpp: %.l
	$(LEX) -o $(@:%.o=%.d)  $<

ast.cpp: ast.cdef 
	$(ASTBUILD) -v outtype=cpp -v outfile=ast.cpp < ast.cdef 

ast.hpp: ast.cdef 
	$(ASTBUILD) -v outtype=hpp -v outfile=ast.hpp < ast.cdef

# source
lexer.o: lexer.cpp parser.hpp ast.hpp
lexer.cpp: lexer.l

parser.o: parser.cpp parser.hpp
parser.cpp: parser.ypp ast.hpp primitive.hpp symtab.hpp

main.o: parser.hpp ast.hpp symtab.hpp primitive.hpp typecheck.cpp codegen.o 
ast2dot.o: parser.hpp ast.hpp symtab.hpp primitive.hpp attribute.hpp

typecheck.o: typecheck.cpp ast.hpp symtab.hpp primitive.hpp attribute.hpp classhierarchy.hpp
codegen.o: codegen.cpp ast.hpp symtab.hpp primitive.hpp attribute.hpp classhierarchy.hpp

ast.o: ast.cpp ast.hpp primitive.hpp symtab.hpp attribute.hpp
ast.cpp: ast.cdef
ast.hpp: ast.cdef

primitive.o: primitive.hpp primitive.cpp ast.hpp

clean:
	rm -f $(RMFILES)
