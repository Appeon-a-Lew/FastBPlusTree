all: btree.a

btree.a: btree.o
	rm -f btree.a
	ar rcs btree.a btree.o

btree-optimized.a: btree-optimized.o
	rm -f btree-optimized.a
	ar rcs btree-optimized.a btree-optimized.o

btree.o:btree.cpp btree.hpp common.h
	clang++ -Wall -Wextra  -O1 -g -c btree.cpp -o $@

btree-optimized.o: btree.cpp btree.hpp common.h
	clang++ -Wall -Wextra -g -c btree.cpp -o $@ -O3 -DNDEBUG

clean:
	rm -f btree.o btree.a btree-optimized.o btree-optimized.a
