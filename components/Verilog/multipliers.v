module mul_4_stage (
	input clk, ce,
	input[31 : 0] a,
	input [31 : 0] b,
	output [31 : 0] p
);

	reg [31 : 0] a_reg = 0, b_reg = 0, q0 = 0, q1 = 0, q2 = 0;

	wire [63 : 0] mul_wire;
	assign mul_wire = a_reg * b_reg;
	wire [31 : 0] mul;
	assign mul = mul_wire[31 : 0];

	
	always @(posedge clk)
		if(ce)begin
			a_reg <= a;
			b_reg <= b;
			q0 <= mul;
			q1 <= q0;
			q2 <= q1;
		end
		
	assign p = q2;

endmodule



module mul_8_stage (
	input clk, ce,
	input[31 : 0] a,
	input [31 : 0] b,
	output [31 : 0] p
);

	reg [31 : 0] a_reg = 0, b_reg = 0, q0 = 0, q1 = 0, q2 = 0, q3 = 0, q4 = 0, q5 = 0, q6 = 0;

	wire [63 : 0] mul_wire;
	assign mul_wire = a_reg * b_reg;
	wire [31 : 0] mul;
	assign mul = mul_wire[31 : 0];
	
	
	always @(posedge clk)
		if(ce)begin
			a_reg <= a;
			b_reg <= b;
			q0 <= mul;
			q1 <= q0;
			q2 <= q1;
			q3 <= q2;
			q4 <= q3;
			q5 <= q4;
			q6 <= q5;
		end
		
	assign p = q6;

endmodule
