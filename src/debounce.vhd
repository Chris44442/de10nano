library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity debounce is port(
  clk : in std_logic;
  sw_i : in std_logic;
  sw_o : out std_logic);
end entity;

architecture rtl of debounce is
  constant LIMIT : integer := 250000; -- Set for 250,000 clock ticks of 25 MHz clock (10 ms)
  signal cnt : integer range 0 to LIMIT := 0;
  signal sw_reg : std_logic := '0';
begin

process(all) is begin
  if rising_edge(clk) then
    -- Switch input is different than internal switch value, so an input is
    -- changing.  Increase counter until it is stable for c_DEBOUNCE_LIMIT.
    if (sw_i /= sw_reg and cnt < LIMIT) then
      cnt <= cnt + 1;
    -- End of counter reached, switch is stable, register it, reset counter
    elsif cnt = LIMIT then
      sw_reg <= sw_i;
      cnt <= 0;
    -- Switches are the same state, reset the counter
    else
      cnt <= 0;
    end if;
  end if;
end process;
sw_o <= sw_reg;
end architecture;
