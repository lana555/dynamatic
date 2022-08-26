# Buffers


## Compile the buffers code from the source repository, where this README lies:

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

```bash
bin/buffers buffers -filename=name -period=period 
```

Here, period indicates target CP (ns).

For example:
```bash
bin/buffers buffers -filename=examples/fir -period=5
```

To see all command options and parameters, run:
```bash
bin/buffers buffers -help
```
