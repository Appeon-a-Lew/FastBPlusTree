# DSE Practical Course Exercise Template
This repository serves as a template for the B-Tree related exercises for the data structure engineering practical course.
Fork this repository and implement your B-Tree in `btree/btree.cpp`.
The current implementation uses `std::map`, your implementation should have identical functionality.
It is recommended that you do not modify `btree/btree.hpp`,
this file defines the interface that our tests expect your btree library to implement.

The most important part of this project is the `btree` static library inside `btree`. This is what you should implement.
You may find the provided tests in `test_main.cpp` useful,
however they are not a complete test suits and will miss many possible bugs.
You should add tests of your own to make sure your implementation is correct.

Feel free to use `make format` to keep your code tidy.

## Week 2 Requirements
- You must implement a B-Tree
  - values are only stored in the leaves
  - You may add next pointers within each level as an optimization
  - You need not support records with a combined key+value size larger than 1024 bytes
  - You must implement split and merge operations, more complicated rebalancing schemes are not mandatory
  - You need not reduce the height of the tree on removal
  - You may implement additional optimizations. For inspiration, see the DSE lecture slides
- your project must be buildable by executing `make` inside the `btree` directory.
  - It must produce a static library `btree/btree.a`
  - The library must define all the symbols in `btree/btree.hpp` so we can run our test by linking against it.
    - If you choose to modify `btree/btree.hpp`, make sure you maintain ABI compatibility.