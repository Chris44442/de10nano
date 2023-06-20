# Neovim Setup

## Your first files

Install Neovim on Ubuntu:

```bash
sudo snap install nvim --classic
```

In `~/.bashrc` write `alias vim='nvim'`.

Create folder and start neovim:

```bash
cd ~/config
mkdir nvim
cd nvim
vim .
```

In Neovim open a new file with `%` called `init.lua`. In it write `print("hello")`.

Create a folder with `d` called `lua`.

Cd into that folder and again with `d` create folder called `<your_name>`.

Cd into that folder as well and open a new file with `%` called `init.lua`. In it write `print("hello from <your_name>")`.

Type `:w` to save and `:Ex`. In the original `init.lua` type `require("<your_name>")`.

## The first remap

