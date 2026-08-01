# libmemleak

C++ header-only library that detects memory leaks when using new/delete.

## Performance

In general libmemleak tests shows 5-6x slow down of program compared to standard new and delete.

## What libmemleak can catch?

libmemleak catches:

1. not freeing memory
2. double free
3. freeing of wild pointers (pointer that was not allocated by 'new')
4. mismatched free

## What libmemleak cannot catch?

1. use-after-frees
2. buffer overflows

## How to test it out?

1. clone repository:

```bash
git clone https://github.com/Naharashu/libmemleak.git
```

2. compile test files:

```bash
g++ leak.cpp -o leak
g++ test.cpp -o test
```

3. run them

```bash
./leak
./test
```

Expected output for leak.cpp:

```bash
Mismatched free(use 'delete[]' instead) of pointer declared at leak.cpp on line 6
Pointer that declared at file leak.cpp at line 5 is not freed
Array that declared at file leak.cpp at line 4 is not freed
Total bytes leaked: 16
Total amount of allocations: 3
```

Expected out for test.cpp:

```bash
Total bytes leaked: 0
Total amount of allocations: 10001
```

## Note

If you will run your program while using libmemleak and valgrind at the same time, this will cause valgrind to show ERROR SUMMARY: number_of_allocation errors. But also it will show that "All heap blocks were freed" and very big amount of bytes allocated. 