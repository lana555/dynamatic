
-----------------------------------------------------------------------
-- int mul wrapper
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity mul_op is
Generic (
 INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
  clk : IN STD_LOGIC;
  rst : IN STD_LOGIC;
  pValidArray : IN std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : OUT std_logic_vector(1 downto 0);
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0));
end entity;

architecture arch of mul_op is

signal join_valid : STD_LOGIC;

-- multiplier latency (4 or 8)
constant LATENCY : integer := 4;
--constant LATENCY : integer := 8;

begin 
	join: entity work.join(arch) generic map(2)
	port map( pValidArray,  
		nReadyArray(0),                        
		join_valid,                  
 		readyArray);   

        -- instantiated multiplier (work.mul_4_stage or work.mul_8_stage)
	multiply_unit:  entity work.mul_4_stage(behav)
	--multiply_unit:  entity work.mul_8_stage(behav)
	port map (
		clk => clk,
		ce => nReadyArray(0),
		a => dataInArray(0),
		b => dataInArray(1),
		p => dataOutArray(0));

	buff: entity work.delay_buffer(arch) generic map(LATENCY)
	port map(clk,
		rst,
		join_valid,
		nReadyArray(0),
		validArray(0));

end architecture;

