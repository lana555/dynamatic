library IEEE;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_textio.all;
use ieee.numeric_std.all;
use std.textio.all;

use work.sim_package.all;



entity adders_tb is

end entity adders_tb;

architecture behav of adders_tb is

	-- Constant declarations

	constant HALF_CLK_PERIOD : TIME := 2.00 ns;
	constant TRANSACTION_NUM : INTEGER := 5;
	constant INPUT_hls_return : STRING := "";
	constant OUTPUT_hls_return : STRING := "../VHDL_OUT/output_hls_return.dat";
	constant DATA_WIDTH_hls_return : INTEGER := 32;
	constant INPUT_A : STRING := "../INPUT_VECTORS/inputA.dat";
	constant OUTPUT_A : STRING := "";
	constant DATA_WIDTH_A : INTEGER := 32;
	constant INPUT_B : STRING := "../INPUT_VECTORS/inputB.dat";
	constant OUTPUT_B : STRING := "";
	constant DATA_WIDTH_B : INTEGER := 32;
	constant INPUT_C : STRING := "../INPUT_VECTORS/inputC.dat";
	constant OUTPUT_C : STRING := "";
	constant DATA_WIDTH_C : INTEGER := 32;
	constant ADDR_WIDTH_C : INTEGER := 2;
	constant DATA_DEPTH_C : INTEGER := 4;
	constant INPUT_D : STRING := "";
	constant OUTPUT_D : STRING := "../VHDL_OUT/output_D.dat";
	constant DATA_WIDTH_D : INTEGER := 32;
	constant INPUT_E : STRING := "";
	constant OUTPUT_E : STRING := "../VHDL_OUT/output_E.dat";
	constant DATA_WIDTH_E : INTEGER := 32;
	constant ADDR_WIDTH_E : INTEGER := 2;
	constant DATA_DEPTH_E : INTEGER := 4;
	constant INPUT_F : STRING := "../INPUT_VECTORS/inputF.dat";
	constant OUTPUT_F : STRING := "../VHDL_OUT/output_F.dat";
	constant DATA_WIDTH_F : INTEGER := 32;
	constant INPUT_G : STRING := "../INPUT_VECTORS/inputG.dat";
	constant OUTPUT_G : STRING := "../VHDL_OUT/output_G.dat";
	constant DATA_WIDTH_G : INTEGER := 32;
	constant ADDR_WIDTH_G : INTEGER := 2;
	constant DATA_DEPTH_G : INTEGER := 4;

	-- Signal declarations

	signal tb_clk : std_logic := '0';
	signal tb_rst : std_logic := '0';
	signal tb_start : std_logic := '0';
	signal tb_done : std_logic;
	signal tb_idle : std_logic;

	signal hls_return_ce : std_logic;
	signal hls_return_we : std_logic;
	signal hls_return_din : std_logic_vector(DATA_WIDTH_hls_return - 1 downto 0);
	signal hls_return_dout : std_logic_vector(DATA_WIDTH_hls_return - 1 downto 0);

	signal A_ce : std_logic;
	signal A_we : std_logic;
	signal A_din : std_logic_vector(DATA_WIDTH_A - 1 downto 0);
	signal A_dout : std_logic_vector(DATA_WIDTH_A - 1 downto 0);

	signal B_ce : std_logic;
	signal B_we : std_logic;
	signal B_din : std_logic_vector(DATA_WIDTH_B - 1 downto 0);
	signal B_dout : std_logic_vector(DATA_WIDTH_B - 1 downto 0);

	signal C_ce : std_logic;
	signal C_we : std_logic;
	signal C_address : std_logic_vector(ADDR_WIDTH_C - 1 downto 0);
	signal C_din : std_logic_vector(DATA_WIDTH_C - 1 downto 0);
	signal C_dout : std_logic_vector(DATA_WIDTH_C - 1 downto 0);

	signal D_ce : std_logic;
	signal D_we : std_logic;
	signal D_din : std_logic_vector(DATA_WIDTH_D - 1 downto 0);
	signal D_dout : std_logic_vector(DATA_WIDTH_D - 1 downto 0);

	signal E_ce : std_logic;
	signal E_we : std_logic;
	signal E_address : std_logic_vector(ADDR_WIDTH_E - 1 downto 0);
	signal E_din : std_logic_vector(DATA_WIDTH_E - 1 downto 0);
	signal E_dout : std_logic_vector(DATA_WIDTH_E - 1 downto 0);

	signal F_ce : std_logic;
	signal F_we : std_logic;
	signal F_din : std_logic_vector(DATA_WIDTH_F - 1 downto 0);
	signal F_dout : std_logic_vector(DATA_WIDTH_F - 1 downto 0);

	signal G_ce : std_logic;
	signal G_we : std_logic;
	signal G_address : std_logic_vector(ADDR_WIDTH_G - 1 downto 0);
	signal G_din : std_logic_vector(DATA_WIDTH_G - 1 downto 0);
	signal G_dout : std_logic_vector(DATA_WIDTH_G - 1 downto 0);


	signal done_or_idle             : STD_LOGIC;
	shared variable transaction_idx : INTEGER := 0;

	-- DUV component

	component adders is
	port(
		clk : in std_logic;
		rst : in std_logic;
		start : in std_logic;
		hls_return_ce : out std_logic;
		hls_return_we : out std_logic;
		hls_return_dout : out std_logic_vector(31 downto 0);
		A_din : in std_logic_vector(31 downto 0);
		B_din : in std_logic_vector(31 downto 0);
		C_ce : out std_logic;
		C_address : out std_logic_vector(1 downto 0);
		C_din : in std_logic_vector(31 downto 0);
		D_ce : out std_logic;
		D_we : out std_logic;
		D_dout : out std_logic_vector(31 downto 0);
		E_ce : out std_logic;
		E_we : out std_logic;
		E_address : out std_logic_vector(1 downto 0);
		E_dout : out std_logic_vector(31 downto 0);
		F_ce : out std_logic;
		F_we : out std_logic;
		F_dout : out std_logic_vector(31 downto 0);
		F_din : in std_logic_vector(31 downto 0);
		G_ce : out std_logic;
		G_we : out std_logic;
		G_address : out std_logic_vector(1 downto 0);
		G_dout : out std_logic_vector(31 downto 0);
		G_din : in std_logic_vector(31 downto 0);
		done : out std_logic;
		idle : out std_logic
	);
	end component adders;

	-- Memory component

	component mem_in_out is
	generic(
		TV_IN : STRING;
		TV_OUT : STRING;
		DATA_WIDTH : INTEGER;
		ADDR_WIDTH : INTEGER;
		DEPTH : INTEGER
	);
	port(
		clk : in std_logic;
		rst : in std_logic;
		ce : in std_logic;
		we : in std_logic;
		address : in std_logic_vector(ADDR_WIDTH - 1 downto 0);
		mem_din : in std_logic_vector(DATA_WIDTH - 1 downto 0);
		mem_dout : out std_logic_vector(DATA_WIDTH - 1 downto 0);
		done : in std_logic
	);
	end component mem_in_out;

	-- Variable component

	component single_variable is
	generic(
		TV_IN : STRING;
		TV_OUT : STRING;
		DATA_WIDTH : INTEGER
	);
	port(
		clk : in std_logic;
		rst : in std_logic;
		ce : in std_logic;
		we : in std_logic;
		mem_din : in std_logic_vector(DATA_WIDTH - 1 downto 0);
		mem_dout : out std_logic_vector(DATA_WIDTH - 1 downto 0);
		done : in std_logic
	);
	end component single_variable;

begin


duv: 	 adders
		port map (
			clk => tb_clk,
			rst => tb_rst,
			start => tb_start,
			done => tb_done,
			idle => tb_idle,
			hls_return_ce => hls_return_ce,
			hls_return_we => hls_return_we,
			hls_return_dout => hls_return_din,
			A_din => A_dout,
			B_din => B_dout,
			C_ce => C_ce,
			C_address => C_address,
			C_din => C_dout,
			D_ce => D_ce,
			D_we => D_we,
			D_dout => D_din,
			E_ce => E_ce,
			E_we => E_we,
			E_address => E_address,
			E_dout => E_din,
			F_ce => F_ce,
			F_we => F_we,
			F_dout => F_din,
			F_din => F_dout,
			G_ce => G_ce,
			G_we => G_we,
			G_address => G_address,
			G_dout => G_din,
			G_din => G_dout
		);


var_inst_hls_return: single_variable 
	generic map(
		TV_IN => INPUT_hls_return,
		TV_OUT => OUTPUT_hls_return,
		DATA_WIDTH => DATA_WIDTH_hls_return
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => hls_return_ce,
		we => hls_return_we,
		mem_din => hls_return_din,
		mem_dout => hls_return_dout,
		done => done_or_idle
	);

var_inst_A: single_variable 
	generic map(
		TV_IN => INPUT_A,
		TV_OUT => OUTPUT_A,
		DATA_WIDTH => DATA_WIDTH_A
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => A_ce,
		we => A_we,
		mem_din => A_din,
		mem_dout => A_dout,
		done => done_or_idle
	);

var_inst_B: single_variable 
	generic map(
		TV_IN => INPUT_B,
		TV_OUT => OUTPUT_B,
		DATA_WIDTH => DATA_WIDTH_B
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => B_ce,
		we => B_we,
		mem_din => B_din,
		mem_dout => B_dout,
		done => done_or_idle
	);

mem_inst_C: mem_in_out 
	generic map(
		TV_IN => INPUT_C,
		TV_OUT => OUTPUT_C,
		DEPTH => DATA_DEPTH_C,
		DATA_WIDTH => DATA_WIDTH_C,
		ADDR_WIDTH => ADDR_WIDTH_C
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => C_ce,
		we => C_we,
		address => C_address,
		mem_din => C_din,
		mem_dout => C_dout,
		done => done_or_idle
	);

var_inst_D: single_variable 
	generic map(
		TV_IN => INPUT_D,
		TV_OUT => OUTPUT_D,
		DATA_WIDTH => DATA_WIDTH_D
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => D_ce,
		we => D_we,
		mem_din => D_din,
		mem_dout => D_dout,
		done => done_or_idle
	);

mem_inst_E: mem_in_out 
	generic map(
		TV_IN => INPUT_E,
		TV_OUT => OUTPUT_E,
		DEPTH => DATA_DEPTH_E,
		DATA_WIDTH => DATA_WIDTH_E,
		ADDR_WIDTH => ADDR_WIDTH_E
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => E_ce,
		we => E_we,
		address => E_address,
		mem_din => E_din,
		mem_dout => E_dout,
		done => done_or_idle
	);

var_inst_F: single_variable 
	generic map(
		TV_IN => INPUT_F,
		TV_OUT => OUTPUT_F,
		DATA_WIDTH => DATA_WIDTH_F
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => F_ce,
		we => F_we,
		mem_din => F_din,
		mem_dout => F_dout,
		done => done_or_idle
	);

mem_inst_G: mem_in_out 
	generic map(
		TV_IN => INPUT_G,
		TV_OUT => OUTPUT_G,
		DEPTH => DATA_DEPTH_G,
		DATA_WIDTH => DATA_WIDTH_G,
		ADDR_WIDTH => ADDR_WIDTH_G
	)
	port map(
		clk => tb_clk,
		rst => tb_rst,
		ce => G_ce,
		we => G_we,
		address => G_address,
		mem_din => G_din,
		mem_dout => G_dout,
		done => done_or_idle
	);



----------------------------------------------------------------------------
-- Write "[[[runtime]]]" and "[[[/runtime]]]" for output transactor
write_output_transactor_hls_return_runtime_proc : process
	file fp             : TEXT;
	variable fstatus    : FILE_OPEN_STATUS;
	variable token_line : LINE;
	variable token      : STRING(1 to 1024);

begin
	file_open(fstatus, fp, OUTPUT_hls_return, WRITE_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_hls_return & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	while transaction_idx /= TRANSACTION_NUM loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	file_open(fstatus, fp, OUTPUT_hls_return, APPEND_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_hls_return & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[/runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	wait;
end process;
write_output_transactor_D_runtime_proc : process
	file fp             : TEXT;
	variable fstatus    : FILE_OPEN_STATUS;
	variable token_line : LINE;
	variable token      : STRING(1 to 1024);

begin
	file_open(fstatus, fp, OUTPUT_D, WRITE_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_D & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	while transaction_idx /= TRANSACTION_NUM loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	file_open(fstatus, fp, OUTPUT_D, APPEND_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_D & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[/runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	wait;
end process;
write_output_transactor_E_runtime_proc : process
	file fp             : TEXT;
	variable fstatus    : FILE_OPEN_STATUS;
	variable token_line : LINE;
	variable token      : STRING(1 to 1024);

begin
	file_open(fstatus, fp, OUTPUT_E, WRITE_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_E & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	while transaction_idx /= TRANSACTION_NUM loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	file_open(fstatus, fp, OUTPUT_E, APPEND_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_E & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[/runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	wait;
end process;
write_output_transactor_F_runtime_proc : process
	file fp             : TEXT;
	variable fstatus    : FILE_OPEN_STATUS;
	variable token_line : LINE;
	variable token      : STRING(1 to 1024);

begin
	file_open(fstatus, fp, OUTPUT_F, WRITE_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_F & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	while transaction_idx /= TRANSACTION_NUM loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	file_open(fstatus, fp, OUTPUT_F, APPEND_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_F & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[/runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	wait;
end process;
write_output_transactor_G_runtime_proc : process
	file fp             : TEXT;
	variable fstatus    : FILE_OPEN_STATUS;
	variable token_line : LINE;
	variable token      : STRING(1 to 1024);

begin
	file_open(fstatus, fp, OUTPUT_G, WRITE_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_G & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	while transaction_idx /= TRANSACTION_NUM loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	file_open(fstatus, fp, OUTPUT_G, APPEND_MODE);
	if (fstatus /= OPEN_OK) then
		assert false report "Open file " & OUTPUT_G & " failed!!!" severity note;
		assert false report "ERROR: Simulation using HLS TB failed." severity failure;
	end if;
	write(token_line, string'("[[[/runtime]]]"));
	writeline(fp, token_line);
	file_close(fp);
	wait;
end process;
----------------------------------------------------------------------------



----------------------------------------------------------------------------

done_or_idle <= tb_done or tb_idle;

----------------------------------------------------------------------------
generate_sim_done_proc : process
begin
	while (transaction_idx /= TRANSACTION_NUM) loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	wait until tb_clk'event and tb_clk = '1';
	assert false report "simulation done!" severity note;
	assert false report "NORMAL EXIT (note: failure is to force the simulator to stop)" severity failure;
	wait;
end process;

----------------------------------------------------------------------------
gen_clock_proc : process
begin
	tb_clk <= '0';
	while (true) loop
		wait for HALF_CLK_PERIOD;
		tb_clk <= not tb_clk;
	end loop;
	wait;
end process;

----------------------------------------------------------------------------
gen_reset_proc : process
begin
	tb_rst <= '1';
	wait for 10 ns;
	tb_rst <= '0';
	wait;
end process;

----------------------------------------------------------------------------
gen_start_proc : process
begin
	tb_start <= '0';
	wait until tb_rst = '0';
	while (tb_idle /= '1') loop
		wait until tb_clk'event and tb_clk = '1';
               wait until tb_clk'event and tb_clk = '1';
	end loop;
	tb_start <= '1';
	wait until tb_idle = '0';
       wait until tb_clk'event and tb_clk = '1';
	tb_start <= '0';
	while (true) loop
		while (tb_done /= '1') loop
			wait until tb_clk'event and tb_clk = '1';
		end loop;
		tb_start <= '1';
		wait until tb_done = '0';
               wait until tb_clk'event and tb_clk = '1';
               wait until tb_clk'event and tb_clk = '1';
		tb_start <= '0';
	end loop;
end process;

----------------------------------------------------------------------------
transaction_increment : process
begin
	wait until tb_rst = '0';
	while (tb_idle /= '1') loop
		wait until tb_clk'event and tb_clk = '1';
	end loop;
	wait until tb_idle = '0';

	while (true) loop
		while (tb_done /= '1') loop
			wait until tb_clk'event and tb_clk = '1';
		end loop;
		transaction_idx := transaction_idx + 1;
		wait until tb_done = '0';
	end loop;
end process;

----------------------------------------------------------------------------


end architecture behav;

