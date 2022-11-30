INCLUDE_DIRS = 
LIB_DIRS = 
#CC = icc
CC = gcc
CPP = g++

CDEFS=
CFLAGS= -g -fopenmp $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT= sequential_CART parallel_CART

HFILES= 
CFILES= 
CPPFILES= sequential_CART.cpp parallel_CART.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~
	-rm -f ${PRODUCT} ${DERIVED} ${GARBAGE}


sequential_CART:	sequential_CART.cpp
	$(CPP) $(CFLAGS) -o $@ sequential_CART.cpp -lm

parallel_CART:	parallel_CART.cpp
	$(CPP) $(CFLAGS) -o $@ parallel_CART.cpp -lm


