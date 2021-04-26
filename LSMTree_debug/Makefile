
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++14 -Wall

all: correctness persistence

correctness: memTable/QuadList.o memTable/SkipList.o memTable/MemTable.o cache.o kvstore.o correctness.o

persistence: QuadList.o SkipList.o kvstore.o persistence.o

clean:
	-rm -f correctness persistence *.o
