# Getting Started

This chapter describes how you can compile your C program to WebAssembly and then run it in WasmEdge runtime.

## Prerequisites

Please make sure you have [wasmedge](https://wasmedge.org/book/en/quick_start/install.html#quick-install) and [emscripten](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions-using-the-emsdk-recommended) installed .

## Hello World

As the first step is always the "Hello World" program , given below is the C program implementation.
```c
// hello.c

#include<stdio.h>
int main(int argc,char **argv){
  printf("Hello World!\n");
  return 0;
}
```

Save it in any file and then compile it to WebAssembly with emscripten .

```bash
emcc hello.c -o hello.wasm
```
> Note: Make sure you either supply `-s STANDALONE_WASM` flag or specify output as wasm `-o your_file_name.wasm`

Then run the wasm in wasmedge runtime

```bash
$ wasmedge hello.wasm
Hello World
```

## Add function

We can also pass cmd arguments , for example add function in this example takes two arguments and prints their sum

```c
// add.c

#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
  int a,b;
  if(argc==3){
    a = atoi(argv[1]);
    b = atoi(argv[2]);
    printf("%d\n",a+b);
  }
  return 0;
}
```

Again Compiling to wasm with emcc

```bash
emcc add.c -o add.wasm
```
Running the application in wasmedge runtime

```bash
$ wasmedge add.wasm 2 3
5
```
We can further improve performance by using wasmedge AOT compilation , this feature gives near native performance

```bash
$ wasmedgec add.wasm add_aot.wasm
$ wasmedge add_aot.wasm 4 9
13
```

## Fibonacci function

We can also structure our project in saperate header and implementation files.

```c
// fibonacci.h

int fib(int n);
```

```c
// fibonacci.c

#include <stdio.h>
#include "fibonacci.h"

int fib(int n){
  int f1 = 0;
  int f2 = 1;
  if(n<=2){
    if(n==1) return f1;
    else return f2;
  }
  else
    for(int i=2; i<n; i++ ){
      int temp = f2;
      f2=f1+f2;
      f1=temp;
    }
  return f2;
}
```

```c
// main.c

#include <stdio.h>
#include <stdlib.h>
#include "fibonacci.h"

int main(int argc, char *argv[])
{
  if (argc<2) {
    return 0;
  }
  int n = atoi(argv[1]);
  printf("%d",fib(n));
  return 0;
}
```

Compiling the program to wasm with emcc
```bash
emcc main.c fibonacci.c -o fib.wasm
```

Running in wasmedge runtime
```bash
$ wasmedge fib.wasm 6
5
```
