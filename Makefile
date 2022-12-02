INCLUDE_DIRS = -I/opt/intel/compilers_and_libraries_2020.0.166/linux/mpi/intel64/include/
LIB_DIRS = -L/opt/intel/compilers_and_libraries_2020.0.166/linux/mpi/intel64/lib/debug -L/opt/intel/compilers_and_libraries_2020.0.166/linux/mpi/intel64/lib
CC = mpicc
CXX = mpicxx
C++ = g++

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
	$(C++) $(CFLAGS) -o $@ sequential_CART.cpp -lm

parallel_CART:	parallel_CART.cpp
	$(CXX) $(CFLAGS) -o $@ parallel_CART.cpp -lm


