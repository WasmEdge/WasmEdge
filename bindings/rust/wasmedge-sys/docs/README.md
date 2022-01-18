
# Guide of Using the Documents

To help you get familiar with the APIs of the wasmedge-sys crate more convenient and efficient, we provide two choices for you. One option is the traditional markdown documents; the other is the interactive Rust notebooks with Evcxr. If you would like to try some pieces of your own code while reading, the second way could be better for you. Of course, you can choose any one of them according to your favorites.

If you prefer the interactive way, you have to do some preparations. So, please read the following contents get things ready before jumping to the documents.

## Preparations for the interactive Rust notebooks

To enable the interactive Rust notebooks on your local machine, you need to make sure the following necessary packages have already been installed:

- Jupyter lab

- EvCxR Jupyter Kernel

- Rust :)

- WasmEdge

If you do not have `Jupyter lab` and `EvCxR Jupyter Kernel` installed yet, just spend 2 minutes reading [this post](https://datacrayon.com/posts/programming/rust-notebooks/setup-anaconda-jupyter-and-rust/) that helps you get them installed on your machine.

If you do not have Rust on your machine :), just reference the [rust-lang official website](https://www.rust-lang.org/tools/install).

The wasmedge-sys crate is the Rust bindings of WasmEdge, so you have to deploy WasmEdge on your machine first. For your convenience, you can use our official docker image. The [Build WasmEdge from source](https://wasmedge.org/book/en/extend/build.html) section in our official document provides all the necessary information to help you get ready.
