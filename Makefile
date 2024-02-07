all:main

btree/btree.a: .FORCE
	cd btree;make btree.a

btree/btree-optimized.a: .FORCE
	cd btree;make btree-optimized.a

only_inner_nodes/btree.a: .FORCE
	cd only_inner_nodes; make btree.a


main: test_main.cpp btree/btree.a tester_btree.hpp PerfEvent.hpp
	clang++ -o $@ -Wall -Wextra -O0 -g $< btree/btree.a


main-inner:  test_main.cpp only_inner_nodes/btree.a tester_btree.hpp PerfEvent.hpp
	clang++ -o $@ -Wall -Wextra -O0 -g $< only_inner_nodes/btree.a


main-optimized: test_main.cpp btree/btree-optimized.a tester_btree.hpp PerfEvent.hpp
	clang++ -o $@ -Wall -Wextra  -g $< btree/btree-optimized.a -O3 -DNDEBUG





format:
	find . -type f -name '*.?pp' -exec clang-format -i {} \;

.PHONY: .FORCE format
.FORCE:

