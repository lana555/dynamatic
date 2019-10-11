----------------------------------------------------------------------- 
-- ret, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity ret_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (0 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(0 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(0 downto 0));
end entity;

architecture arch of ret_op is

begin 

    dataOutArray(0) <= dataInArray(0);
    validArray(0) <= pValidArray(0);
    readyArray(0) <= nReadyArray(0);

end architecture;

----------------------------------------------------------------------- 
-- int add, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity add_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of add_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- int sub, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity sub_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of sub_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) - unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- int mul, version 0.0
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

-- Interface to Vivado component
component array_RAM_mul_32sbkb_MulnS_0 is
  port (
        clk : IN STD_LOGIC;
        ce : IN STD_LOGIC;
        a : IN STD_LOGIC_VECTOR;
        b : IN STD_LOGIC_VECTOR;
        p : OUT STD_LOGIC_VECTOR);
end component;

signal q0, q1, q2, q3: std_logic;
signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    multiply_unit :  component array_RAM_mul_32sbkb_MulnS_0
    port map (
        clk => clk,
        ce => nReadyArray(0),
        a => dataInArray(0),
        b => dataInArray(1),
        p => dataOutArray(0));

    process (clk) is
    begin
      if rising_edge(clk) then  
        if (rst = '1') then
          q0 <= join_valid;
          q1 <= '0';
          q2 <= '0';
          q3 <= '0';
        elsif (nReadyArray(0)='1') then
          q0 <= join_valid;
          q1 <= q0;
          q2 <= q1;
          q3 <= q2;
        end if;
      end if;
    end process;

    validArray(0) <= q3;

end architecture;

-----------------------------------------------------------------------
-- logic and, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity and_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of and_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0) and dataInArray(1);
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- logic or, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity or_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of or_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0) or dataInArray(1);
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- logic xor, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity xor_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of xor_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0) xor dataInArray(1);
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- sext, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity sext_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of sext_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( 
                pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(IEEE.numeric_std.resize(signed(dataInArray(0)),signed(dataInArray(1))));
    validArray(0) <= join_valid;

end architecture;


-----------------------------------------------------------------------
-- zext, version 0.0
-- TODO check signed/unsigned
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity zext_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (0 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(0 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(0 downto 0));
end entity;

architecture arch of zext_op is

    signal join_valid : STD_LOGIC;

begin 

    dataOutArray(0)<= std_logic_vector(IEEE.numeric_std.resize(signed(dataInArray(0)),DATA_SIZE_OUT));
    validArray <= pValidArray;
    readyArray(0) <= not pValidArray(0) or (pValidArray(0) and nReadyArray(0));

end architecture;
-----------------------------------------------------------------------
-- shl, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity shl_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of shl_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(shift_left(unsigned(dataInArray(0)),to_integer(unsigned('0' & dataInArray(1)(DATA_SIZE_IN-2 downto 0)))));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- ashr, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity ashr_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of ashr_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(shift_right(signed(dataInArray(0)),to_integer(unsigned('0' & dataInArray(1)(DATA_SIZE_IN-2 downto 0)))));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- lshr, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity lshr_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of lshr_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(shift_right(unsigned(dataInArray(0)),to_integer(unsigned('0' & dataInArray(1)(DATA_SIZE_IN-2 downto 0)))));
    validArray(0) <= join_valid;


end architecture;

-----------------------------------------------------------------------
-- select, version 0.0
-----------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity antitokens is port(
  clk, reset : in std_logic;
  pvalid1, pvalid0 : in std_logic;
  kill1, kill0 : out std_logic;
  generate_at1, generate_at0 : in std_logic;
  stop_valid : out std_logic);

end antitokens;


architecture arch of antitokens is

signal reg_in0, reg_in1, reg_out0, reg_out1 : std_logic;
  
begin

  reg0 : process(clk, reset, reg_in0)
    begin
      if(reset='1') then
        reg_out0 <= '0'; 
      else
        if(rising_edge(clk))then
          reg_out0 <= reg_in0;
        end if;
      end if;
    end process reg0;

  reg1 : process(clk, reset, reg_in1)
    begin
      if(reset='1') then
        reg_out1 <= '0'; 
      else
        if(rising_edge(clk))then
          reg_out1 <= reg_in1;
        end if;
      end if;
    end process reg1;

  reg_in0 <= not pvalid0 and (generate_at0 or reg_out0);
  reg_in1 <= not pvalid1 and (generate_at1 or reg_out1);

  stop_valid <= reg_out0 or reg_out1;

  kill0 <= generate_at0 or reg_out0;
  kill1 <= generate_at1 or reg_out1;

end arch;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
USE work.customTypes.all;

entity select_op is 
Generic (
 INPUTS : integer ; OUTPUTS : integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port (
    clk, rst :      in std_logic;
    dataInArray :   in data_array (1 downto 0)(DATA_SIZE_IN - 1 downto 0);
    dataOutArray :  out data_array (0 downto 0)(DATA_SIZE_OUT - 1 downto 0);    
    pValidArray :   in std_logic_vector(2 downto 0);
    nReadyArray :   in std_logic_vector(0 downto 0);
    validArray :    out std_logic_vector(0 downto 0);
    readyArray :    out std_logic_vector(2 downto 0);
    condition: in data_array (0 downto 0)(0 downto 0)
  );

  end select_op;

architecture arch of select_op is
    signal  ee, validInternal : std_logic;
    signal  kill0, kill1 : std_logic;
  signal  antitokenStop:  std_logic;
  signal  g0, g1: std_logic;


    begin
      
      ee <= pValidArray(0) and ((condition(0)(0) and pValidArray(2)) or (not condition(0)(0) and pValidArray(1))); --condition and one input
      validInternal <= ee and not antitokenStop; -- propagate ee if not stopped by antitoken

      g0 <= not pValidArray(1) and validInternal and nReadyArray(0);
      g1 <= not pValidArray(2) and validInternal and nReadyArray(0);

      validArray(0) <= validInternal;
      readyArray(1) <= (not pValidArray(1)) or (validInternal and nReadyArray(0)) or kill0; -- normal join or antitoken
      readyArray(2) <= (not pValidArray(2)) or (validInternal and nReadyArray(0)) or kill1; --normal join or antitoken
      readyArray(0) <= (not pValidArray(0)) or (validInternal and nReadyArray(0)); --like normal join

      dataOutArray(0) <= dataInArray(1) when (condition(0)(0) = '1') else dataInArray(0);

      Antitokens: entity work.antitokens
        port map ( clk, rst, 
        pValidArray(2), pValidArray(1), 
              kill1, kill0,
              g1, g0, 
              antitokenStop);
    end arch;


-----------------------------------------------------------------------
-- icmp eq, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_eq_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_eq_op is
--eq: yields true if the operands are equal, false otherwise. No sign interpretation is necessary or performed.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (dataInArray(0) = dataInArray(1) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ne, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ne_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ne_op is
--ne: yields true if the operands are unequal, false otherwise. No sign interpretation is necessary or performed.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= zero when (dataInArray(0) = dataInArray(1) ) else one;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ugt, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ugt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ugt_op is
--ugt: interprets the operands as unsigned values and yields true if op1 is greater than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) > unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp uge, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_uge_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_uge_op is
--uge: interprets the operands as unsigned values and yields true if op1 is greater than or equal to op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) >= unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp sgt, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_sgt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_sgt_op is
-- sgt: interprets the operands as signed values and yields true if op1 is greater than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";
    signal res: std_logic_vector (0 downto 0);
    signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) > signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp sge, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_sge_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_sge_op is
-- sge: interprets the operands as signed values and yields true if op1 is greater than or equal to op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";
    signal res: std_logic_vector (0 downto 0);
    signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) >= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ult, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ult_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ult_op is
--ugt: interprets the operands as unsigned values and yields true if op1 is greater than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) < unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ule, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ule_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ule_op is
-- ule: interprets the operands as unsigned values and yields true if op1 is less than or equal to op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) <= unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp slt, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_slt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_slt_op is
-- slt: interprets the operands as signed values and yields true if op1 is less than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) < signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp sle, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_sle_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_sle_op is
-- slt: interprets the operands as signed values and yields true if op1 is less than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- getelementptr, version 0.0
-- for now, only wrapper! need to fix for multi-dimensional arrays
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity getelementptr_op_generic is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (INPUTS - 1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of getelementptr_op_generic is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(INPUTS)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0);
    validArray(0) <= join_valid;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;
entity getelementptr_op is
Generic ( INPUT_SIZE: integer; INPUT_SIZE1 : Integer; OUTPUT_SIZE: Integer; DATA_SIZE : Integer);
port(
  clk : IN STD_LOGIC;
  rst : IN STD_LOGIC;
  pValidArray : IN std_logic_vector(1 downto 0);
  nReadyArray : IN STD_LOGIC_VECTOR(0 downto 0);
  validArray : OUT STD_LOGIC_VECTOR(0 downto 0);
  readyArray : OUT std_logic_vector(1 downto 0);
  dataInArray: IN data_array(INPUT_SIZE-1 downto 0)(31 DOWNTO 0);
  dataOutArray : OUT data_array(0 downto 0)(31 DOWNTO 0));
end entity;


architecture arch of  getelementptr_op is

    signal join_valid : STD_LOGIC;

    signal tmp_6_fu_144_p2 : STD_LOGIC_VECTOR (9 downto 0);
    signal p_shl_cast_fu_128_p1 : STD_LOGIC_VECTOR (10 downto 0);
    signal p_shl1_cast_fu_140_p1 : STD_LOGIC_VECTOR (10 downto 0);
    signal tmp_2_fu_120_p3 : STD_LOGIC_VECTOR (9 downto 0);
    signal tmp_5_fu_132_p3 : STD_LOGIC_VECTOR (5 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    --dataOutArray(0) <= std_logic_vector(IEEE.numeric_std.resize(unsigned(dataInArray(0)),OUTPUT_SIZE));
    validArray(0) <= join_valid;

tmp_2_fu_120_p3 <= (dataInArray(0)(4 downto 0) & "00000");
tmp_5_fu_132_p3 <= (dataInArray(0)(4 downto 0) & '0');

tmp_6_fu_144_p2 <= std_logic_vector(unsigned(tmp_2_fu_120_p3) - unsigned(tmp_5_fu_132_p3));

dataOutArray(0) <= "000000000000000000000" & std_logic_vector(unsigned(tmp_6_fu_144_p2) + unsigned(dataInArray(1)(10 downto 0)));
end architecture;


-----------------------------------------------------------------------
-- fcmp oeq, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_floatCmp is

port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(0 DOWNTO 0));
end entity;

architecture olt of ALU_floatCmp is


    --component array_RAM_fcmp_32cud IS 
component array_RAM_fcmp_32dEe IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;


begin



   -- array_RAM_fcmp_32cud_U2 : component array_RAM_fcmp_32cud
array_RAM_fcmp_32cud_U2 : component array_RAM_fcmp_32dEe
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
 clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        opcode => "00100",
        dout => dout);


    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
          
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
             
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

architecture ogt of ALU_floatCmp is


    component array_RAM_fcmp_32dEe IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;


begin



    array_RAM_fcmp_32cud_U2 : component array_RAM_fcmp_32dEe
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
 clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        opcode => "00010",
        dout => dout);


    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
          
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
             
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_oeq is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_oeq is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ogt, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ogt is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ogt is

     signal add_out: STD_LOGIC_VECTOR (0 downto 0);
    signal add_valid, add_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      add_ready,     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    add: entity work.ALU_floatCmp (ogt)
            port map (clk, rst,
                     join_valid,     --pvalid,
               nReadyArray(0),         --nready,
                     add_valid, --valid,
                     add_ready, --ready,
               dataInArray(0),           --din0
               dataInArray(1),           --din1
                     add_out);  --dout

   -- dataOutArray(0) <= add_out;
    --validArray(0) <= add_valid;


    process(clk, rst) is

          begin

           if (rst = '1') then

             validArray(0)  <= '0';
             dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (add_valid= '1') then
                  validArray(0)   <= '1';
                  dataOutArray(0) <= add_out;
                        
                    else
                      if (nReadyArray(0) = '1') then
                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                    end if;
                   end if;
                        
            
             end if;
    end process; 

end architecture;

-----------------------------------------------------------------------
-- fcmp oge, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_oge is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_oge is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp olt, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_olt is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_olt is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ole, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ole is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ole is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp one, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_one is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_one is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ord, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ord is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ord is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp uno, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_uno is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_uno is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ueq, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ueq is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ueq is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp uge, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_uge is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_uge is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ult, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ult is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ult is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ule, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ule is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ule is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp une, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_une is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_une is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- fcmp ugt, version 0.0
-- TODO
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ugt is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ugt is

    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- float add, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_floatAdd is

port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of ALU_floatAdd is




    component array_RAM_fadd_32bkb IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR (31 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;


begin



    array_RAM_fadd_32bkb_U1 : component array_RAM_fadd_32bkb
    generic map (
        ID => 1,
        NUM_STAGE => 10,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
 clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);


    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
                q5 <= '0';
                q6 <= '0';
        q7 <= '0';
                q8 <= '0';
               
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                q5 <= q4;
                q6 <= q5;
        q7 <= q6;
                q8 <= q7;
             end if;
          end if;
       end process;

       valid <= q8;

end architecture;


Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fadd is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fadd is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

     multiply: entity work.ALU_floatAdd (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
               nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
               dataInArray(0),           --din0
               dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
    validArray(0) <= alu_valid;

end architecture;



----------------------------------------------------------------------- 
-- float sub, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_floatSub is

port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of ALU_floatSub is




    component array_RAM_fsub_32bkb IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR (31 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;


begin



    array_RAM_fsub_32bkb_U1 : component array_RAM_fsub_32bkb
    generic map (
        ID => 1,
        NUM_STAGE => 10,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
 clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);


    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
                q5 <= '0';
                q6 <= '0';
        q7 <= '0';
                q8 <= '0';
               
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                q5 <= q4;
                q6 <= q5;
        q7 <= q6;
                q8 <= q7;
             end if;
          end if;
       end process;

       valid <= q8;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fsub is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fsub is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

     multiply: entity work.ALU_floatSub (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
               nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
               dataInArray(0),           --din0
               dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
    validArray(0) <= alu_valid;

end architecture;


----------------------------------------------------------------------- 
-- float mul, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity float_mul is

port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of float_mul is
    component array_RAM_mul_32s_32s_32_7_MulnS_0 is
    port(
     clk : IN STD_LOGIC;
            ce : IN STD_LOGIC;
            a : IN STD_LOGIC_VECTOR;
            b : IN STD_LOGIC_VECTOR;
            p : OUT STD_LOGIC_VECTOR);
    end component;

component array_RAM_fmul_32cud IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
port(
 clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR (31 downto 0) );
    end component;





    signal q0, q1, q2, q3, q4, q5, q6: std_logic;


begin

 array_RAM_fmul_32cud_U2 : component array_RAM_fmul_32cud
    generic map (
        ID => 1,
        NUM_STAGE => 6,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
 clk =>  clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);


    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q4;

end architecture;


Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fmul is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fmul is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

     multiply: entity work.float_mul (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
               nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
               dataInArray(0),           --din0
               dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
    validArray(0)<= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- unsigned int division, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity udiv is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of udiv is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- signed int division, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity sdiv is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of sdiv is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- float division, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fdiv is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fdiv is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- srem, remainder of signed division, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity srem is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of srem is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- urem, remainder of unsigned division, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity urem is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of urem is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- frem, remainder of float division, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity frem is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
 clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of frem is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;
