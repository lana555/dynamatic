library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;
use IEEE.numeric_std.all;
use ieee.std_logic_textio.all;
use std.textio.all;
use work.sim_package.all;

----------------------------------------------------------------------------

entity single_argument is
	generic(
		TV_IN      : STRING  := "";
		TV_OUT     : STRING  := "";
		DATA_WIDTH : INTEGER := 32
	);
	port(
		clk       : IN  STD_LOGIC;
		rst       : IN  STD_LOGIC;
		ce0       : IN  STD_LOGIC;
		we0       : IN  STD_LOGIC;
		mem_dout0 : OUT STD_LOGIC_VECTOR(DATA_WIDTH - 1 downto 0);
		mem_din0  : IN  STD_LOGIC_VECTOR(DATA_WIDTH - 1 downto 0);
		done      : IN  STD_LOGIC      
	);
end single_argument;

----------------------------------------------------------------------------

architecture behav of single_argument is

	-- Inner signals 

	shared variable mem : STD_LOGIC_VECTOR(DATA_WIDTH - 1 downto 0) := (others => '0');
	signal write_done_flag : STD_LOGIC := '0';

begin
----------------------------------------------------------------------------
-----------------------------------Read array-------------------------------
	-- Read data from file to array


	read_file_proc : process
		file fp                  : TEXT;
		variable fstatus         : FILE_OPEN_STATUS;
		variable token_line      : LINE;
		variable token           : STRING(1 to 128);
		variable token_len       : INTEGER;
		variable token_int       : INTEGER;
		variable transaction_idx : INTEGER;
		variable idx             : INTEGER;

	begin
		if (TV_IN /= "") then
		
		wait until rst = '0';
		transaction_idx := 0;
		file_open(fstatus, fp, TV_IN, READ_MODE);
		if (fstatus /= OPEN_OK) then
			assert false report "Open file " & TV_IN & " failed!!!" severity failure;
		end if;
		esl_read_token(fp, token_line, token);
		if (token(1 to 13) /= "[[[runtime]]]") then
			assert false report "ERROR: Simulation using HLS TB failed." severity failure;
		end if;
		esl_read_token(fp, token_line, token);
		while (token(1 to 14) /= "[[[/runtime]]]") loop
			if (token(1 to 15) /= "[[transaction]]") then
				assert false report "ERROR: Simulation using HLS TB failed." severity failure;
			end if;
			esl_read_token(fp, token_line, token); -- Skip transaction number
			-- Start to read data for every transaction round
			while ( done /= '1') loop  
				wait until clk'event and clk = '1';
			end loop;

			esl_read_token(fp, token_line, token);
			mem := esl_str2lv_hex(token, DATA_WIDTH);

			esl_read_token(fp, token_line, token);
			wait until done = '0';

			if (token(1 to 16) /= "[[/transaction]]") then
				assert false report "ERROR: Simulation using HLS TB failed." severity failure;
			end if;
			esl_read_token(fp, token_line, token);
			transaction_idx := transaction_idx + 1;
		end loop;
		file_close(fp);
		
		end if;
		wait;
	end process;
----------------------------------------------------------------------------
	-- Read data from array to RTL
	read_proc : process(clk, rst)
	begin
		if (rst = '1') then
			mem_dout0 <= (others => '0');
		elsif (clk'event and clk = '1') then
			if (ce0 = '1') then
				mem_dout0 <= mem after 0.1 ns;
			end if;
		end if;
	end process;

----------------------Write array-------------------------------------------

	--Write data from RTL to array
	write_proc : process(clk)
	begin
		if (clk'event and clk = '1') then
			if (ce0 = '1' and we0 = '1') then
				mem := mem_din0;
			end if;
		end if;
	end process;
----------------------------------------------------------------------------
	-- Write data from array to file
	write_file_proc : process
		file fp                  : TEXT;
		variable fstatus         : FILE_OPEN_STATUS;
		variable token_line      : LINE;
		variable token           : STRING(1 to 128);
		variable transaction_idx : INTEGER;
	begin
		if (TV_OUT /= "") then

		wait until (rst = '0');
		transaction_idx := 0;
		while (done /= '1') loop
			wait until clk'event and clk = '1';
		end loop;
		wait until done = '0';          

		while (true) loop
			while (done /= '1') loop
				wait until clk'event and clk = '1';
			end loop;
			file_open(fstatus, fp, TV_OUT, APPEND_MODE);
			if (fstatus /= OPEN_OK) then
				assert false report "Open file " & TV_OUT & " failed!!!" severity failure;
			end if;
			write(token_line, "[[transaction]]    " & integer'image(transaction_idx));
			writeline(fp, token_line);

			write(token_line, "0x" & esl_conv_string_hex(mem));
			writeline(fp, token_line);

			write(token_line, string'("[[/transaction]]"));
			writeline(fp, token_line);
			transaction_idx := transaction_idx + 1;
			file_close(fp);
			wait until done = '0';
		end loop;
		end if;
	wait;
	end process;
----------------------------------------------------------------------------
end behav;
