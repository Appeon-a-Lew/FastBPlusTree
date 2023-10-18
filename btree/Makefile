all: btree.a

btree.a: btree.o
	ar rcs btree.a btree.o

btree.o:btree.cpp btree.hpp
	clang++ -Wall -Wextra -g -c btree.cpp -o $@

clean:
	rm -f btree.o btree.a
