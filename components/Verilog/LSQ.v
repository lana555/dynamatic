//----------------------------------------------------------------------- 
//-- LSQ Load Operation, version 0.0
//-----------------------------------------------------------------------
//As always Address is in 1 and data is in 0


//Wrong implementation. There will be problem when data size and address size are not equal
module lsq_load_op #(parameter INPUTS = 2,
		parameter OUTPUTS = 2,
		parameter DATA_SIZE = 32,
		parameter ADDRESS_SIZE = 32)
	(
		input clk,
		input rst,
		input [INPUTS * (DATA_SIZE)- 1 : 0]data_in_bus,
		input [INPUTS - 1 : 0]valid_in_bus,
		output [INPUTS - 1 : 0] ready_in_bus,
		
		output [OUTPUTS * (DATA_SIZE) - 1 : 0]data_out_bus,
		output [OUTPUTS - 1 : 0]valid_out_bus,
		input 	[OUTPUTS - 1 : 0] ready_out_bus
);
	
	assign data_out_bus[1 * DATA_SIZE +: DATA_SIZE] = data_in_bus[1 * DATA_SIZE +: DATA_SIZE];
	assign valid_out_bus[1] = valid_in_bus[1];
	assign ready_in_bus[1] = ready_out_bus[1];
	
	assign data_out_bus[0 * DATA_SIZE +: DATA_SIZE] = data_in_bus[0 * DATA_SIZE +: DATA_SIZE];
	assign valid_out_bus[0] = valid_in_bus[0];
	assign ready_in_bus[0] = ready_out_bus[0];
endmodule




//----------------------------------------------------------------------- 
//-- LSQ Store Operation, version 0.0
//-----------------------------------------------------------------------
//As always Address is in 1 and data is in 0

//Wrong implementation. There will be problem when data size and address size are not equal
module lsq_store_op #(parameter INPUTS = 2,
		parameter OUTPUTS = 2,
		parameter DATA_SIZE = 32,
		parameter ADDRESS_SIZE = 32)
	(
		input clk,
		input rst,
		input [INPUTS * (DATA_SIZE)- 1 : 0]data_in_bus,
		input [INPUTS - 1 : 0]valid_in_bus,
		output [INPUTS - 1 : 0] ready_in_bus,
		
		output [OUTPUTS * (DATA_SIZE) - 1 : 0]data_out_bus,
		output [OUTPUTS - 1 : 0]valid_out_bus,
		input 	[OUTPUTS - 1 : 0] ready_out_bus
);
	
	assign data_out_bus[1 * DATA_SIZE +: DATA_SIZE] = data_in_bus[1 * DATA_SIZE +: DATA_SIZE];
	assign valid_out_bus[1] = valid_in_bus[1];
	assign ready_in_bus[1] = ready_out_bus[1];
	
	assign data_out_bus[0 * DATA_SIZE +: DATA_SIZE] = data_in_bus[0 * DATA_SIZE +: DATA_SIZE];
	assign valid_out_bus[0] = valid_in_bus[0];
	assign ready_in_bus[0] = ready_out_bus[0];
endmodule

