CC = clang++
CFLAGS = -Wall -Wextra -g
SRC = btree.c
OBJ = $(SRC:.c=.o)

all:main

btree/btree.a: .FORCE
	cd btree;rm btree.a;make btree.a

main: test_main.cpp btree/btree.a
	clang++ -o $@ -Wall -Wextra -g $< btree/btree.a

format:
	find . -type f -name '*.?pp' -exec clang-format -i {} \;

.PHONY: .FORCE format
.FORCE:

