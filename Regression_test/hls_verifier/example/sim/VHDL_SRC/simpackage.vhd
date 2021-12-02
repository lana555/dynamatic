library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.numeric_std.all;
use std.textio.all;
use IEEE.std_logic_unsigned.all;
use ieee.std_logic_textio.all;
----------------------------------------------------------------------------
package sim_package is

	procedure esl_read_token(file textfile : TEXT;
	                         textline      : inout LINE;
	                         token         : out STRING;
	                         token_len     : out INTEGER);
	procedure esl_read_token(file textfile : TEXT;
	                         textline      : inout LINE;
	                         token         : out STRING);
	function esl_str2lv_hex(RHS : STRING; data_width : INTEGER) return STD_LOGIC_VECTOR;
	function esl_conv_string_hex(lv : STD_LOGIC_VECTOR) return STRING;
	function esl_str_dec2int(RHS : STRING) return INTEGER;

end package;
----------------------------------------------------------------------------
package body sim_package is

----------------------------------------------------------------------------
--read tockens from files
	procedure esl_read_token(file textfile : TEXT; textline : inout LINE; token : out STRING; token_len : out INTEGER) is
		variable whitespace : CHARACTER;
		variable i          : INTEGER;
		variable ok         : BOOLEAN;
		variable buff       : STRING(1 to token'length);
	begin
		ok        := false;
		i         := 1;
		loop_main : while not endfile(textfile) loop
			if textline = null or textline'length = 0 then
				readline(textfile, textline);
			end if;
			loop_remove_whitespace : while textline'length > 0 loop
				if textline(textline'left) = ' ' or textline(textline'left) = HT or textline(textline'left) = CR or textline(textline'left) = LF then
					read(textline, whitespace);
				else
					exit loop_remove_whitespace;
				end if;
			end loop;
			loop_aesl_read_token : while textline'length > 0 and i <= buff'length loop
				if textline(textline'left) = ' ' or textline(textline'left) = HT or textline(textline'left) = CR or textline(textline'left) = LF then
					exit loop_aesl_read_token;
				else
					read(textline, buff(i));
					i := i + 1;
				end if;
				ok := true;
			end loop;
			if ok = true then
				exit loop_main;
			end if;
		end loop;
		buff(i)   := ' ';
		token     := buff;
		token_len := i - 1;
	end procedure esl_read_token;

----------------------------------------------------------------------------
--read token from files
	procedure esl_read_token(file textfile : TEXT;
	                         textline      : inout LINE;
	                         token         : out STRING) is
		variable i : INTEGER;
	begin
		esl_read_token(textfile, textline, token, i);
	end procedure esl_read_token;

----------------------------------------------------------------------------
-- String to hex conversion
	function esl_str2lv_hex(RHS : STRING; data_width : INTEGER) return STD_LOGIC_VECTOR is
		variable ret : STD_LOGIC_VECTOR(data_width - 1 downto 0);
		variable idx : integer := 3;
	begin
		ret := (others => '0');
		if (RHS(1) /= '0' and (RHS(2) /= 'x' or RHS(2) /= 'X')) then
			report "Error! The format of hex number is not initialed by 0x";
		end if;
		while true loop
			if (data_width > 4) then
				case RHS(idx) is
					when '0'       => ret := ret(data_width - 5 downto 0) & "0000";
					when '1'       => ret := ret(data_width - 5 downto 0) & "0001";
					when '2'       => ret := ret(data_width - 5 downto 0) & "0010";
					when '3'       => ret := ret(data_width - 5 downto 0) & "0011";
					when '4'       => ret := ret(data_width - 5 downto 0) & "0100";
					when '5'       => ret := ret(data_width - 5 downto 0) & "0101";
					when '6'       => ret := ret(data_width - 5 downto 0) & "0110";
					when '7'       => ret := ret(data_width - 5 downto 0) & "0111";
					when '8'       => ret := ret(data_width - 5 downto 0) & "1000";
					when '9'       => ret := ret(data_width - 5 downto 0) & "1001";
					when 'a' | 'A' => ret := ret(data_width - 5 downto 0) & "1010";
					when 'b' | 'B' => ret := ret(data_width - 5 downto 0) & "1011";
					when 'c' | 'C' => ret := ret(data_width - 5 downto 0) & "1100";
					when 'd' | 'D' => ret := ret(data_width - 5 downto 0) & "1101";
					when 'e' | 'E' => ret := ret(data_width - 5 downto 0) & "1110";
					when 'f' | 'F' => ret := ret(data_width - 5 downto 0) & "1111";
					when 'x' | 'X' => ret := ret(data_width - 5 downto 0) & "XXXX";
					when ' '       => return ret;
					when others => report "Wrong hex char " & RHS(idx);
						return ret;
				end case;
			elsif (data_width = 4) then
				case RHS(idx) is
					when '0'       => ret := "0000";
					when '1'       => ret := "0001";
					when '2'       => ret := "0010";
					when '3'       => ret := "0011";
					when '4'       => ret := "0100";
					when '5'       => ret := "0101";
					when '6'       => ret := "0110";
					when '7'       => ret := "0111";
					when '8'       => ret := "1000";
					when '9'       => ret := "1001";
					when 'a' | 'A' => ret := "1010";
					when 'b' | 'B' => ret := "1011";
					when 'c' | 'C' => ret := "1100";
					when 'd' | 'D' => ret := "1101";
					when 'e' | 'E' => ret := "1110";
					when 'f' | 'F' => ret := "1111";
					when 'x' | 'X' => ret := "XXXX";
					when ' '       => return ret;
					when others => report "Wrong hex char " & RHS(idx);
						return ret;
				end case;
			elsif (data_width = 3) then
				case RHS(idx) is
					when '0'       => ret := "000";
					when '1'       => ret := "001";
					when '2'       => ret := "010";
					when '3'       => ret := "011";
					when '4'       => ret := "100";
					when '5'       => ret := "101";
					when '6'       => ret := "110";
					when '7'       => ret := "111";
					when 'x' | 'X' => ret := "XXX";
					when ' '       => return ret;
					when others => report "Wrong hex char " & RHS(idx);
						return ret;
				end case;
			elsif (data_width = 2) then
				case RHS(idx) is
					when '0'       => ret := "00";
					when '1'       => ret := "01";
					when '2'       => ret := "10";
					when '3'       => ret := "11";
					when 'x' | 'X' => ret := "XX";
					when ' '       => return ret;
					when others => report "Wrong hex char " & RHS(idx);
						return ret;
				end case;
			elsif (data_width = 1) then
				case RHS(idx) is
					when '0'       => ret := "0";
					when '1'       => ret := "1";
					when 'x' | 'X' => ret := "X";
					when ' '       => return ret;
					when others => report "Wrong hex char " & RHS(idx);
						return ret;
				end case;
			else
				report string'("Wrong data_width.");
				return ret;
			end if;
			idx := idx + 1;
		end loop;
		return ret;
	end function;
----------------------------------------------------------------------------
-- Hex to string conversion
	
function esl_conv_string_hex(lv : STD_LOGIC_VECTOR) return STRING is
		constant str_len   : integer := (lv'length + 3) / 4;
		variable ret       : STRING(1 to str_len);
		variable i, tmp    : INTEGER;
		variable normal_lv : STD_LOGIC_VECTOR(lv'length - 1 downto 0);
		variable tmp_lv    : STD_LOGIC_VECTOR(3 downto 0);
	begin
		normal_lv := lv;
		for i in 1 to str_len loop
			if (i = 1) then
				if ((lv'length mod 4) = 3) then
					tmp_lv(2 downto 0) := normal_lv(lv'length - 1 downto lv'length - 3);
					case tmp_lv(2 downto 0) is
						when "000"  => ret(i) := '0';
						when "001"  => ret(i) := '1';
						when "010"  => ret(i) := '2';
						when "011"  => ret(i) := '3';
						when "100"  => ret(i) := '4';
						when "101"  => ret(i) := '5';
						when "110"  => ret(i) := '6';
						when "111"  => ret(i) := '7';
						when others => ret(i) := 'X';
					end case;
				elsif ((lv'length mod 4) = 2) then
					tmp_lv(1 downto 0) := normal_lv(lv'length - 1 downto lv'length - 2);
					case tmp_lv(1 downto 0) is
						when "00"   => ret(i) := '0';
						when "01"   => ret(i) := '1';
						when "10"   => ret(i) := '2';
						when "11"   => ret(i) := '3';
						when others => ret(i) := 'X';
					end case;
				elsif ((lv'length mod 4) = 1) then
					tmp_lv(0 downto 0) := normal_lv(lv'length - 1 downto lv'length - 1);
					case tmp_lv(0 downto 0) is
						when "0"    => ret(i) := '0';
						when "1"    => ret(i) := '1';
						when others => ret(i) := 'X';
					end case;
				elsif ((lv'length mod 4) = 0) then
					tmp_lv(3 downto 0) := normal_lv(lv'length - 1 downto lv'length - 4);
					case tmp_lv(3 downto 0) is
						when "0000" => ret(i) := '0';
						when "0001" => ret(i) := '1';
						when "0010" => ret(i) := '2';
						when "0011" => ret(i) := '3';
						when "0100" => ret(i) := '4';
						when "0101" => ret(i) := '5';
						when "0110" => ret(i) := '6';
						when "0111" => ret(i) := '7';
						when "1000" => ret(i) := '8';
						when "1001" => ret(i) := '9';
						when "1010" => ret(i) := 'a';
						when "1011" => ret(i) := 'b';
						when "1100" => ret(i) := 'c';
						when "1101" => ret(i) := 'd';
						when "1110" => ret(i) := 'e';
						when "1111" => ret(i) := 'f';
						when others => ret(i) := 'X';
					end case;
				end if;
			else
				tmp_lv(3 downto 0) := normal_lv((str_len - i) * 4 + 3 downto (str_len - i) * 4);
				case tmp_lv(3 downto 0) is
					when "0000" => ret(i) := '0';
					when "0001" => ret(i) := '1';
					when "0010" => ret(i) := '2';
					when "0011" => ret(i) := '3';
					when "0100" => ret(i) := '4';
					when "0101" => ret(i) := '5';
					when "0110" => ret(i) := '6';
					when "0111" => ret(i) := '7';
					when "1000" => ret(i) := '8';
					when "1001" => ret(i) := '9';
					when "1010" => ret(i) := 'a';
					when "1011" => ret(i) := 'b';
					when "1100" => ret(i) := 'c';
					when "1101" => ret(i) := 'd';
					when "1110" => ret(i) := 'e';
					when "1111" => ret(i) := 'f';
					when others => ret(i) := 'X';
				end case;
			end if;
		end loop;
		return ret;
	end function;
----------------------------------------------------------------------------
	--character to decimal conversion
function esl_str_dec2int(RHS : STRING) return INTEGER is
		variable ret : integer;
		variable idx : integer := 1;
	begin
		ret := 0;
		while true loop
			case RHS(idx) is
				when '0' => ret := ret * 10 + 0;
				when '1' => ret := ret * 10 + 1;
				when '2' => ret := ret * 10 + 2;
				when '3' => ret := ret * 10 + 3;
				when '4' => ret := ret * 10 + 4;
				when '5' => ret := ret * 10 + 5;
				when '6' => ret := ret * 10 + 6;
				when '7' => ret := ret * 10 + 7;
				when '8' => ret := ret * 10 + 8;
				when '9' => ret := ret * 10 + 9;
				when ' ' => return ret;
				when others => report "Wrong dec char " & RHS(idx);
					return ret;
			end case;
			idx := idx + 1;
		end loop;
		return ret;
	end esl_str_dec2int;
----------------------------------------------------------------------------
end package body;

