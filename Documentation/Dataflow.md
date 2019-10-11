#A DOT-based specification language for Dataflow Networks
This document describes the specification language for Dataflow Networks (DFNs). The syntax is based on the [DOT](https://graphviz.gitlab.io/_pages/doc/info/lang.html) language from [graphviz](http://www.graphviz.org/).

## Dataflow Networks
A DFN consists of **blocks** and **channels**. It is represented by a _digraph_ where each _node_ corresponds to a block and each _edge_ corresponds to a channel. Blocks have **input** and **output ports**. Channels are unidirectional and connect an output port from one block to an input port of another block.

Every port and channel have a width that indicates the bitwidth of the data transferred through the channel. 0-width channels are used to implement synchronization, i.e., transfer of events that do not carry data.


### Blocks

DFNs have a set of building blocks sufficient to implement any behavior. The current set of DF blocks is the following:

* **Operator**: it implements the arithmetic/logic operations. Operators can be either combinational or sequential.
* **Buffer**: it is used for data storage. Buffers operate as FIFOs with a specific capacity.
* **Constant**: it is used to generate constant values when required by some operators.
* **Fork**: it behaves like a one-input many-output operator that produces a copy of the input to each output.
* **Merge**: it has multiple inputs (mutually exclusive) and one output. The arrival of information to any input is transferred to the output.
* **Select**: It behaves as a multiplexer that can select between one of the two inputs based on the value of a condition.
* **Branch**: It behaves as a demultiplexer and selects one of the two outputs to transfer the data at the input depending on the value of a condition.
* **Demux**: It is a multi-output demultiplexer in which the data at the input is transferred to one of the outputs. Each output port has an associated input control port. The control ports are mutually exclusive.
* **Entry**: It is a control block used to implement one of the entries of the DFN.
* **Exit**: It is a control block used to implement one of the exits of the DFN.
* **Source**: A block which infitely issues tokens to a single successor.
* **Sink**: A block which infinitely consumes tokens from a single predecessor.
* **MC**: Memory interface, simple arbiter which issues loads and stores to memory.
* **LSQ**: Memory interface, load-store queue which ensures in-order load-store accesses.
* **Mux**: More complex version of Merge, ensures correct input ordering.

In its simplest form, a block is specified by declaring its type and the list of input and output ports. For example, a block with two input ports (_a_ and _b_)
and two output ports (_x_ and _y_) would be declared as follows:

> ```F [type=Operator, in="a b", out="x y"];```

**Note**: in general, the items in the lists within quotes will be space-separated.

#### Port width

The width of the ports associated to a block can be specified in two ways:

* By adding a suffix to the port
* By associating the default width of the ports

The graph DFN specification can have a global attribute declared as follows:

> ```channel_width=32;```

The previous declaration indicates that the default width for all the ports is 32. The width of each port can also be specified individually in the declaration of the block, e.g.,

> ```F [type=Operator, in="a:32 b:16", out="x:32 y:1"];```

#### Ports for flow control

Some blocks have ports with a specific semantics related to conditional flow control (select, branch and demux). The ports are identified with special suffixes in their declaration.

>***Select and Branch***

>Here is an example:

>> ```sel [type=Select, in="a+:64 b-:64 sel?:1", out="z:64"];```

>Three ports have special semantics in a select block: the _condition_ (suffix ?) and the ports selected when true (suffix +) or false (suffix -).

>Similarly, a branch block also identifies the condition and the selected output ports with the same suffixes, e.g.,

>> ```br [type=Branch, in="d:32 sel?:1", out="outT+:32 outF-:32"];```


>***Demux***

>A demux block has _n_ outputs and _n+1_ inputs. Here, the order of declaration is important. The first _n_ inputs ports are the control ports that select the output ports. The last input port is the input data. For example:

>> ```demux [type=Demux, in="c1:0 c2:0 c3:0 d:16", out="d1:16 d2:16 d3:16"];```

In the previous example, ports _c1_, _c2_ and _c3_ are associated with _d1_, _d2_ and _d3_, respectively, while input port _d_ is the one carrying the data that will be trasferred to one of the outputs.
 
#### Delays

The delay of every block can be specified with the attribute _delay_. A zero delay is assumed when not specified.

The simplest specification of delay is by assigning a value to the block, e.g.,

> ```B [in="in1 in2", out="out1 out2", delay=12.5];```

indicating that the input-to-output delay is 12.5 time units.

Delays can also be assigned to individual ports, thus enabling different port-to-port delays. For example:

>```B [in="in1 in2", out="out1 out2", delay="in1:5 12.5 out2:10"];```

In the previous example, the unprefixed delay corresponds to the delay of the block. The prefixed delays are associated to the ports. Thus, the delay from _in1_ to _out2_ is 27.5 (5+12.5+10), whereas the delay from _in1_ to _out1_ is 17.5 (5+12.5). Finally, the delay from _in2_ to _out1_ is 12.5.

#### Pipelined units

Blocks of type _Operator_ can also be pipelined. The behavior of a pipelined unit is determined by two parameters: latency and initiation interval. Both are measured in _"number of cycles"_. By default, latency is assumed to be zero, which implies that the block is combinational. When _latency > 0_, the initiation interval determines the minimum number of cycles that must be executed between the initiation of consecutive operations.

Here is an example of specification of a pipelined unit:

>```P [type=Operator, in="in1 in2", out="out1 out2", latency=2, II=1];```

The _delay_ attribute has a particular interpretation for pipelined units. The delays associated to the input ports represent the delays from the port to the internal registers of the block. Similarly, the delays associated to the output ports represent the delays from the internal registers to the port. Finally, the delay of the block represents the internal register-to-register delays of the block. This delay is a constraint for the cycle period of the system.

#### Elastic Buffers

Elastic Buffers are characterized by two parameters: _size_ and _transparency_. The size represents the number of slots to store data. Transparency indicates whether the buffer can be by-passed or not. A transparent buffer has a combinational path from input to output and only stores data in case of back-pressure.

Here is an example of specification of an elastic buffer:

>``` buf [type=Buffer, in="input:16", out="output:16", slots=2, transparent=false];```

### Channels

Channels connect one output port to one input port. The syntax is as follows:

> ```block1 -> block2 [from=out2, to=in1]; ```

The previous declaration indicates that a channel connects port *out2* from *block1* to port *in1* from *block2*.

### Other Attributes

There are some other attributes we need to produce a functionally correct VHDL netlist:

* **op**: arithmetic/memory instruction (i.e., add, sub, load)
* **value**: constant value in hex
* **bbID, portID, offset**: load/store parameters, used for connecting appropriately to the LSQ/MC
* **memory, bbcount**: LSQ/MC parameters, for generating the correct memory interface (which is application-specific). The ports of LSQ and MC have some additional port attributes, which are written after  ```*``` and denote the port type and properties 





