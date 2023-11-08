all:main

btree/btree.a: .FORCE
	cd btree;make btree.a

btree/btree-optimized.a: .FORCE
	cd btree;make btree-optimized.a

main: test_main.cpp btree/btree.a tester.hpp PerfEvent.hpp
	clang++ -o $@ -Wall -Wextra -g $< btree/btree.a

main-optimized: test_main.cpp btree/btree-optimized.a tester.hpp PerfEvent.hpp
	clang++ -o $@ -Wall -Wextra -g $< btree/btree-optimized.a -O3 -DNDEBUG

format:
	find . -type f -name '*.?pp' -exec clang-format -i {} \;

.PHONY: .FORCE format
.FORCE:

