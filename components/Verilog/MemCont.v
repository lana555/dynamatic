module read_write_priority #(parameter ARBITER_SIZE = 1) (
	input [ARBITER_SIZE - 1 : 0] req,
	input [ARBITER_SIZE - 1 : 0] data_ready,
	output reg [ARBITER_SIZE - 1 : 0] priority_out = 0
);

	reg prio_req = 0;
	integer i;
	
	always@(req, data_ready)begin
		priority_out[0] = req[0] & data_ready[0];
		prio_req = req[0] & data_ready[0];
		
		for(i = 1; i <= ARBITER_SIZE - 1; i = i + 1)begin
			priority_out[i] = ~prio_req & req[i] & data_ready[i];
			prio_req = prio_req | (req[i] & data_ready[i]);
		end
	end

endmodule


module read_write_address_mux #(parameter ARBITER_SIZE = 1, ADDR_WIDTH = 32) (
	input [ARBITER_SIZE - 1 : 0] sel,
	input [ARBITER_SIZE * ADDR_WIDTH - 1 : 0] addr_in,
	output reg [ADDR_WIDTH - 1 : 0] addr_out = 0
);

	integer i;
	
	always @(*)begin
		addr_out = 0;
		for(i = 0; i <= ARBITER_SIZE - 1; i = i + 1)
			if(sel[i])
				addr_out = addr_in[i * ADDR_WIDTH +: ADDR_WIDTH];
	end

endmodule


module read_write_address_ready #(parameter ARBITER_SIZE = 1) (
	input [ARBITER_SIZE - 1 : 0] sel,
	input [ARBITER_SIZE - 1 : 0] nReady,
	output [ARBITER_SIZE - 1 : 0] ready
);

	assign ready = nReady & sel;

endmodule


module read_data_signals #(parameter ARBITER_SIZE = 1, DATA_WIDTH = 32) (
	input clk, rst,
	input [ARBITER_SIZE - 1 : 0] sel,
	input [DATA_WIDTH - 1 : 0] read_data,
	output reg [ARBITER_SIZE * DATA_WIDTH  - 1 : 0] out_data = 0,
	output reg [ARBITER_SIZE - 1 : 0] valid = 0,
	input [ARBITER_SIZE - 1 : 0] nReady
);
	reg [ARBITER_SIZE - 1 : 0] sel_prev = 0;
	reg [ARBITER_SIZE * DATA_WIDTH  - 1 : 0] out_reg = 0;
	
	integer i;
	
	always @(posedge clk, posedge rst)begin
		if(rst)begin
			valid <= 0;
			sel_prev <= 0;
		end 
		else begin
			sel_prev <= sel;
			for(i = 0; i <= ARBITER_SIZE - 1; i = i + 1)
				if(sel[i])
					valid[i] <= 1;
				else if(nReady[i])
					valid[i] <= 0;
		end
	end
	
	always @(posedge clk)begin
		for(i = 0; i <= ARBITER_SIZE - 1; i = i + 1)
			if(sel_prev[i])
				out_reg[i * DATA_WIDTH +: DATA_WIDTH] <= read_data;
	end
	
	always @(*)begin
		for(i = 0; i <= ARBITER_SIZE - 1; i = i + 1)begin
			if(sel_prev[i])
				out_data[i * DATA_WIDTH +: DATA_WIDTH] = read_data;
			else
				out_data[i * DATA_WIDTH +: DATA_WIDTH] = out_reg[i * DATA_WIDTH +: DATA_WIDTH];
		end
	end
	
endmodule



module read_memory_arbiter #(parameter ARBITER_SIZE = 2, ADDR_WIDTH = 32, DATA_WIDTH = 32) (
	input clk, rst,
	
	input [ARBITER_SIZE - 1 : 0] pValid,
	output [ARBITER_SIZE - 1 : 0] ready,
	input [ARBITER_SIZE * ADDR_WIDTH  - 1 : 0] address_in,
	
	input [ARBITER_SIZE - 1 : 0] nReady,
	output [ARBITER_SIZE - 1 : 0] valid,
	output [ARBITER_SIZE * DATA_WIDTH  - 1 : 0] data_out,
	
	output read_enable,
	output [ADDR_WIDTH - 1 : 0] read_address,
	input [DATA_WIDTH - 1 : 0] data_from_memory
);

	wire [ARBITER_SIZE - 1 : 0]priorityOut;
	
	read_write_priority #(.ARBITER_SIZE(ARBITER_SIZE)) priority(.req(pValid), .data_ready(nReady), .priority_out(priorityOut));
	
	read_write_address_mux #(.ARBITER_SIZE(ARBITER_SIZE), .ADDR_WIDTH(ADDR_WIDTH)) address_mux (.sel(priorityOut), .addr_in(address_in), .addr_out(read_address));
	
	read_write_address_ready #(.ARBITER_SIZE(ARBITER_SIZE)) address_ready (.sel(priorityOut), .nReady(nReady), .ready(ready));
	
	read_data_signals #(.ARBITER_SIZE(ARBITER_SIZE), .DATA_WIDTH(DATA_WIDTH)) data_signal (.rst(rst), .clk(clk), .sel(priorityOut), .read_data(data_from_memory), .out_data(data_out), .valid(valid), .nReady(nReady));
	
	assign read_enable = | priorityOut;

endmodule



module write_data_signals #(parameter ARBITER_SIZE = 1, DATA_WIDTH = 32) (
	input clk, rst,
	input [ARBITER_SIZE - 1 : 0] sel,
	output reg [DATA_WIDTH - 1 : 0] write_data = 0,
	input [ARBITER_SIZE * DATA_WIDTH  - 1 : 0] in_data,
	output reg [ARBITER_SIZE - 1 : 0] valid
);

	integer i;
	
	always @(*)begin
		write_data = 0;
		for(i = 0; i <= ARBITER_SIZE - 1; i = i + 1)
			if(sel[i])
				write_data = in_data[i * DATA_WIDTH +: DATA_WIDTH];
	end
	
	always @(posedge clk, posedge rst)begin
		if(rst)
			valid <= 0;
		else valid <= sel;
	end
	
endmodule


module write_memory_arbiter #(parameter ARBITER_SIZE = 2, ADDR_WIDTH = 32, DATA_WIDTH = 32) (
	input clk, rst,
	
	input [ARBITER_SIZE - 1 : 0] pValid,
	output [ARBITER_SIZE - 1 : 0] ready,
	input [ARBITER_SIZE * ADDR_WIDTH  - 1 : 0] address_in,
	input [ARBITER_SIZE * DATA_WIDTH  - 1 : 0] data_in,
	
	input [ARBITER_SIZE - 1 : 0] nReady,
	output [ARBITER_SIZE - 1 : 0] valid,
	
	
	output write_enable,
	output enable,
	output [ADDR_WIDTH - 1 : 0] write_address,
	output [DATA_WIDTH - 1 : 0] data_to_memory
);

	wire [ARBITER_SIZE - 1 : 0]priorityOut;
	
	read_write_priority #(.ARBITER_SIZE(ARBITER_SIZE)) priority(.req(pValid), .data_ready(nReady), .priority_out(priorityOut));
	
	read_write_address_mux #(.ARBITER_SIZE(ARBITER_SIZE), .ADDR_WIDTH(ADDR_WIDTH)) address_mux (.sel(priorityOut), .addr_in(address_in), .addr_out(write_address));
	
	read_write_address_ready #(.ARBITER_SIZE(ARBITER_SIZE)) address_ready (.sel(priorityOut), .nReady(nReady), .ready(ready));
	
	write_data_signals #(.ARBITER_SIZE(ARBITER_SIZE), .DATA_WIDTH(DATA_WIDTH)) data_signal (.rst(rst), .clk(clk), .sel(priorityOut), .write_data(data_to_memory), .in_data(data_in), .valid(valid));
	
	assign write_enable = | priorityOut;
	assign enable = | priorityOut;
	
endmodule



module MemCont #(parameter DATA_SIZE = 32, ADDRESS_SIZE = 32, BB_COUNT = 1, LOAD_COUNT = 1, STORE_COUNT = 1) (
	input clk, rst,
	
	output [DATA_SIZE - 1 : 0] io_storeDataOut,
	output [ADDRESS_SIZE - 1 : 0] io_storeAddrOut,
	output io_storeEnable,
	
	input [DATA_SIZE - 1 : 0] io_loadDataIn,
	output [ADDRESS_SIZE - 1 : 0] io_loadAddrOut,
	output io_loadEnable,
	
	input [BB_COUNT - 1 : 0] io_bbpValids,
	input [BB_COUNT * 32 - 1 : 0] io_bb_stCountArray,
	output [BB_COUNT - 1 : 0] io_bbReadyToPrevs,
	
	output io_Empty_Valid,
	input io_Empty_Ready,
	
	input [LOAD_COUNT - 1 : 0] io_rdPortsPrev_valid,
	input [LOAD_COUNT * ADDRESS_SIZE - 1 : 0] io_rdPortsPrev_bits,
	output [LOAD_COUNT - 1 : 0] io_rdPortsPrev_ready,
	
	output [LOAD_COUNT - 1 : 0] io_rdPortsNext_valid,
	output [LOAD_COUNT * ADDRESS_SIZE - 1 : 0] io_rdPortsNext_bits,
	input [LOAD_COUNT - 1 : 0] io_rdPortsNext_ready,
	
	input [STORE_COUNT - 1 : 0] io_wrAddrPorts_valid,
	input [STORE_COUNT * ADDRESS_SIZE - 1 : 0] io_wrAddrPorts_bits,
	output [STORE_COUNT - 1 : 0] io_wrAddrPorts_ready,
	
	input [STORE_COUNT - 1 : 0] io_wrDataPorts_valid,
	input [STORE_COUNT * DATA_SIZE - 1 : 0] io_wrDataPorts_bits,
	output [STORE_COUNT - 1 : 0] io_wrDataPorts_ready
);

	wire [STORE_COUNT - 1 : 0]valid_WR;
	
	assign io_wrDataPorts_ready = io_wrAddrPorts_ready;
	
	read_memory_arbiter #(.ARBITER_SIZE(LOAD_COUNT), .ADDR_WIDTH(ADDRESS_SIZE), .DATA_WIDTH(DATA_SIZE)) read_arbiter (
		.rst(rst), .clk(clk),
		
		.pValid(io_rdPortsPrev_valid), .ready(io_rdPortsPrev_ready), .address_in(io_rdPortsPrev_bits), 
		.nReady(io_rdPortsNext_ready), .valid(io_rdPortsNext_valid), .data_out(io_rdPortsNext_bits),
		.read_enable(io_loadEnable), .read_address(io_loadAddrOut), .data_from_memory(io_loadDataIn) 
	);

	write_memory_arbiter #(.ARBITER_SIZE(STORE_COUNT), .ADDR_WIDTH(ADDRESS_SIZE), .DATA_WIDTH(DATA_SIZE)) write_arbiter (
		.rst(rst), .clk(clk),
		
		.pValid(io_wrAddrPorts_valid), .ready(io_wrAddrPorts_ready), .address_in(io_wrAddrPorts_bits), 
		.data_in(io_wrDataPorts_bits), .nReady({STORE_COUNT{1'b1}}), .valid(valid_WR),
		.write_enable(io_storeEnable), .write_address(io_storeAddrOut), .data_to_memory(io_storeDataOut) 
	);
	
	reg [31:0] counter = 0;

	
	integer i;
	
	always @(posedge clk)begin
		if(rst)
			counter = 0;
		else begin
			for(i = 0; i <= BB_COUNT - 1; i = i + 1)
				if(io_bbpValids[i])
					counter = counter + io_bb_stCountArray[i * 32 +: 32];
					
			if(io_storeEnable)
				counter = counter - 1;
		end
	end
	
	assign io_Empty_Valid = (~(|counter)) & (~(|io_bbpValids));
	
	assign io_bbReadyToPrevs = {BB_COUNT{1'b1}};
	
endmodule


module elasticBufferDummy #(parameter INPUTS = 1, OUTPUTS = 1, DATA_IN_SIZE = 32, DATA_OUT_SIZE = 32) (
	input clk, rst,
	input [DATA_IN_SIZE - 1 : 0]dataInArray,
	output [DATA_OUT_SIZE - 1 : 0]dataOutArray,
	output ready, valid,
	input pValid, nReady
);
	assign dataOutArray = dataInArray;
	assign ready = nReady;
	assign valid = pValid;
endmodule

//Address in in index 1 and data is in index 0
module mc_load_op #(parameter INPUTS = 2, OUTPUTS = 2, ADDRESS_SIZE = 32, DATA_SIZE = 32) (
	input clk, rst,
	
	input [INPUTS - 1 : 0]valid_in_bus,		//Two valids by default. one for address other for data
	output [INPUTS - 1 : 0] ready_in_bus,
	input [DATA_SIZE - 1 : 0] data_in_bus,
	input [ADDRESS_SIZE - 1 : 0] address_in_bus,
	
	output [OUTPUTS - 1 : 0]valid_out_bus,	//Two valids by default. one for address other for data
	input [OUTPUTS - 1 : 0] ready_out_bus,
	output [DATA_SIZE - 1 : 0] data_out_bus,
	output [ADDRESS_SIZE - 1 : 0] address_out_bus

);

	wire Buffer_1_readyArray_0;
	wire Buffer_1_validArray_0;
	wire [ADDRESS_SIZE - 1 : 0] Buffer_1_dataOutArray_0;
	
	wire Buffer_2_readyArray_0;
	wire Buffer_2_validArray_0;
	wire [DATA_SIZE - 1 : 0] Buffer_2_dataOutArray_0;
	
	wire [ADDRESS_SIZE - 1 : 0] addr_from_circuit;
	wire addr_from_circuit_valid;
	wire addr_from_circuit_ready;
	
	wire [ADDRESS_SIZE - 1 : 0] addr_to_lsq;
	wire addr_to_lsq_valid;
	wire addr_to_lsq_ready;

	wire [DATA_SIZE - 1 : 0] data_from_lsq;
	wire data_from_lsq_valid;
	wire data_from_lsq_ready;

	wire [DATA_SIZE - 1 : 0] data_to_circuit;
	wire data_to_circuit_valid;
	wire data_to_circuit_ready;
	
	assign addr_from_circuit = address_in_bus;
	assign addr_from_circuit_valid = valid_in_bus[1];
	assign ready_in_bus[1] = Buffer_1_readyArray_0;
	
	TEHB #(.INPUTS(1), .OUTPUTS(1), .DATA_IN_SIZE(ADDRESS_SIZE), .DATA_OUT_SIZE(ADDRESS_SIZE)) buffer1 (
		.clk(clk), .rst(rst),
		.data_in_bus(addr_from_circuit), .valid_in_bus(addr_from_circuit_valid), .ready_in_bus(Buffer_1_readyArray_0),
		.data_out_bus(Buffer_1_dataOutArray_0), .valid_out_bus(Buffer_1_validArray_0), .ready_out_bus(addr_to_lsq_ready)
	);
	
	assign addr_to_lsq = Buffer_1_dataOutArray_0;
	assign addr_to_lsq_valid = Buffer_1_validArray_0;
	assign addr_to_lsq_ready = ready_out_bus[1];
	
	assign address_out_bus = addr_to_lsq;
	assign valid_out_bus[1] = addr_to_lsq_valid;
	
	assign ready_in_bus[0] = data_from_lsq_ready;
	
	assign data_from_lsq = data_in_bus;
	assign data_from_lsq_valid = valid_in_bus[0];
	assign data_from_lsq_ready = Buffer_2_readyArray_0;
	
	assign data_out_bus = Buffer_2_dataOutArray_0;
	assign valid_out_bus[0] = Buffer_2_validArray_0;
	
	
	TEHB #(.INPUTS(1), .OUTPUTS(1), .DATA_IN_SIZE(DATA_SIZE), .DATA_OUT_SIZE(DATA_SIZE)) buffer2 (
		.clk(clk), .rst(rst),
		.data_in_bus(data_from_lsq), .valid_in_bus(data_from_lsq_valid), .ready_in_bus(Buffer_2_readyArray_0),
		.data_out_bus(Buffer_2_dataOutArray_0), .valid_out_bus(Buffer_2_validArray_0), .ready_out_bus(ready_out_bus[0])
	);
	
endmodule



module mc_store_op #(parameter INPUTS = 2, OUTPUTS = 2, ADDRESS_SIZE = 32, DATA_SIZE = 32) (
	input clk, rst, 
	
	input [ADDRESS_SIZE - 1 : 0] address_in_bus,
	input [DATA_SIZE - 1 : 0] data_in_bus,
	input [INPUTS - 1 : 0] valid_in_bus,
	output [INPUTS - 1 : 0] ready_in_bus,
	
	output [OUTPUTS - 1 : 0]valid_out_bus,
	input [OUTPUTS - 1 : 0] ready_out_bus,
	output [DATA_SIZE - 1 : 0] data_out_bus,
	output [ADDRESS_SIZE - 1 : 0] address_out_bus
	
);

	wire single_ready;
	wire join_valid;
	
	
	joinC #(.N(2)) join_write (.valid_in(valid_in_bus), .ready_in(ready_in_bus), .valid_out(join_valid), .ready_out(ready_out_bus[0]));

	assign data_out_bus = data_in_bus;
	assign valid_out_bus = {OUTPUTS{join_valid}};
	assign address_out_bus = address_in_bus;

endmodule













