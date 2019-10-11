# Dot2VHDL


## Compile the dot2vhdl code from the source repository, where this README lies:

```bash
mkdir bin
make
```

## Install dependencies

In order to run the Buffers command you will need to install graphviz, graphviz-dev and pkg-config
(if not already installed), which you can do with the following commands:

```bash
sudo apt install graphviz
sudo apt install graphviz-dev
sudo apt install pkg-config
```

You will also need to install the cbc (Coin-or branch and cut) MILP solver, which you can do with the following command:

```bash
sudo apt install coinor-cbc
```

## Run buffer opt

Usage: buffers buffers period buffer_delay solver infile outfile

```bash
bin/buffers buffers 3 0.0 cbc examples/fir_graph.dot examples/_build/fir_graph.dot
bin/buffers shab period 0.0 cbc set timeout dataflow_in.dot bbgraph_in.dot dataflow_out.dot bbgraph_out.dot 0
```
Here, period indicates target CP (ns), set indicates presence (1) or absence of set optimization, timeout is the MILP timeout (ms).

For example:
```bash
bin/buffers shab 10 0.0 cbc 1 20 examples/example.dot examples/example_bbgraph.dot example_10.dot example_bbgraph.dot 0
```

Check out the files generated in `examples`.


