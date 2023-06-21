library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity up_dn_counter_top is
  port ( clk : in  std_logic;     -- input clock
                                  -- leds to display count
         led : out  std_logic_vector (7 downto 0);
         dir : in  std_logic);    -- direction of counter (up or down)
end up_dn_counter_top;

architecture behavioral of up_dn_counter_top is
  signal clk_div : std_logic_vector (3 downto 0) := x"0";
  signal count   : std_logic_vector (7 downto 0) := x"00";
begin

    -- clock divider
  process (clk)
  begin
    if (clk'event and clk = '1') then
      clk_div <= clk_div + '1';
    end if;
  end process;

    -- up/down counter
  process (clk_div(3), dir)
  begin
    if (clk_div(3)'event and clk_div(3) = '1') then
      if (dir = '1') then
        count <= count + '1';   -- counting up
      elsif (dir = '0') then
        count <= count - '1';   -- counting down
      end if;
    end if;
  end process;

    -- display count on leds
  led <= not count;

end behavioral;

