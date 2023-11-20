# why the late submission

## git problems
I tried and tried to push but could not becuase of the merge conflicts. So on the 20th did a hard reset and pushed. see the screenshot. Sorry for the inconvenience. 

## known problems

- double free: during deleting the upper is deleted twice 
- empty string: empty string is breaking 
- slow scan: the scan is incredibly slow :/






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

## Week 4 Requirements
- Implement at least one of the proposed optimizations for your B-Tree
- make sure `test.sh` runs without errors
- you may get an idea of your implementation's performance using `perf.sh`
- you should add more tests than what `test_main.cpp` does, it will miss lots of stuff

On an AWS t3.medium, you should be getting at least 500k lookups/second with `INT=5e7`.
If you are measuring locally, the performance you can obtain depends on your hardware.
Most will have better results than the t3.medium.
On my Desktop, I get around 2.2M lookups/s, for instance.
We're working on getting you access to a university provided server for your benchmarking.



