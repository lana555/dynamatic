# Dynamatic: Dynamically Scheduled HLS

## Build the Elastic Pass

### Build and install LLVM 

```bash
git clone http://github.com/llvm-mirror/llvm --branch release_60 --depth 1
cd llvm/tools
git clone http://github.com/llvm-mirror/clang --branch release_60 --depth 1
git clone http://github.com/llvm-mirror/polly --branch release_60 --depth 1
cd ..
mkdir _build && cd _build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DLLVM_INSTALL_UTILS=ON -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=$HOME/llvm-6.0
make
make install
```

### Compile the elastic pass code from the source repository, where this README lies:

```bash
mkdir _build && cd _build
cmake .. -DLLVM_ROOT=$HOME/llvm-6.0
make
```

--------------------------------------------------------------

## Run the Elastic Pass

```bash
cd examples/
vim Makefile
```

Change the lines below according to which installation of LLVM you are using:
```makefile
CLANG = ../../llvm-6.0/bin/clang # change depending on where your installation of clang is
OPT = ../../llvm-6.0/bin/opt # change depending on where your installation of opt is
```

Use this option together with function addBuffersSimple (in MyCFGPass.cpp) to obtain a naive buffer placement:
```bash
make name=loop_array graph
xdg-open _build/loop_array_graph.png
```

Use this option together without function addBuffersSimple (in MyCFGPass.cpp) to obtain a design without buffers (which can then be optimized with Buffers):
```bash
make name=loop_array graph_f
xdg-open _build/loop_array_graph.png
```
Note: this option requires a main() and a function call. Still not present in many examples.

Ta-da!

Check out the files generated in `_build`.
