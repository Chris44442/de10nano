library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity counter is
port (
  clk : in std_logic;
  led : out std_logic_vector (7 downto 0);
  dir : in std_logic);
end counter;

architecture rtl of counter is
  signal clk_div : std_logic_vector (3 downto 0) := 4x"0";
  signal count : std_logic_vector (7 downto 0) := 8x"00";
begin

process (all) begin
  if (clk'event and clk = '1') then
    clk_div <= clk_div + '1';
  end if;
end process;

process (all) begin
  if (clk_div(3)'event and clk_div(3) = '1') then
    if (dir = '1') then
      count <= count + '1';
    elsif (dir = '0') then
      count <= count - '1';
    end if;
  end if;
end process;

led <= not count;

end rtl;

