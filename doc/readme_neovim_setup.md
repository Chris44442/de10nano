# Neovim Setup

## Installation

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

Replace the nvim folder with something like `nvim_primeagen_final` for a basic setup. It needs a package manager to run.

## Packer

In .config do:

```
git clone --depth 1 https://github.com/wbthomason/packer.nvim\
 ~/.local/share/nvim/site/pack/packer/start/packer.nvim
```

## Keybindings

abc
:so
:PackerSync

:wq :q!   leader_pv  leader_pf  leader_ps  ctrl-P

Echo syntax group name of word under cursor:
:echo synIDattr(synID(line("."), col("."), 1), "name")
