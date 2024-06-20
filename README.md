# Not Named Compiler
Compiler that generates bad but working x86_64 Intel assembly.<br>
Does not uses any auto tools or ready-to-use virtual machines.<br>
Done by hand.<br>

## Dependencies
This compiler only supported on Linux and requires<br>
GNU Linker (`ld`) and GNU Assembler (`as`) to be present.<br>
If you want to run auto tests, you'll also need `python` interpreter.

## How to build
You can easily build this with CMake, see how:</br>
```
git clone https://github.com/almeswe/nnc.git
mkdir nnc/src/build && cd nnc/src/build
as --64 ../nnc_rt.s -o rt.o
cmake .. && make -j
```
You can also select build version (`Debug` or `Release`) in CMakeLists.txt file.<br>
```
SET(CMAKE_BUILD_TYPE ???)
```
If it is not specified, you'll build `Release` version.

## Make sure it works
Current version of this project uses automated black-box tests located at [`./py_test`](./py_test).<br>
These tests ensure general correctness of the generation, and represent robust way to test compilers. (based on [`this`](https://github.com/c-testsuite/c-testsuite)).<br>
You can run these tests using following command from `.../nnc/py_test` folder:<br>
```
python nnc_gen_test.py tests
```
Don't waste your time on [`./tests`](./tests).<br> 

## Play around
See working examples at [`./examples`](./examples).<br>
I left build instructions inside each project's folder (or separate file).

## Future goals
Are you interested in this project? See in what i'm currently [`dived in`](./TODO.md).

## License
Project is published under [`MIT`](./LICENSE) License.