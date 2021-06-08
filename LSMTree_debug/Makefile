
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++14 -Wall

all: correctness persistence

correctness: memTable/QuadList.o memTable/SkipList.o memTable/MemTable.o cache.o kvstore.o SSTable.o correctness.o

persistence: memTable/QuadList.o memTable/SkipList.o memTable/MemTable.o  cache.o kvstore.o SSTable.o persistence.o

clean:
	-rm -f correctness persistence *.o memTable/*.o
