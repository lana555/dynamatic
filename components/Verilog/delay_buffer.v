module delay_buffer #(parameter SIZE = 32) (
	input clk, rst,
	input valid_in, ready_in,
	output valid_out
);

	reg [SIZE - 1 : 0] shift_reg = 0;
	
	always @(posedge clk)begin
		if(ready_in | rst)
			shift_reg[0] <= valid_in;
	end
	
	always @(posedge clk)begin
		if(rst)
			shift_reg[SIZE - 1 : 1] <= 0;
		else if(ready_in)
			shift_reg[SIZE - 1 : 1] <= shift_reg[SIZE - 2 : 0];
	end
	
	assign valid_out = shift_reg[SIZE - 1];

endmodule
