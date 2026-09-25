# dwmblocksc - modular status bar for dwm

dwmblocksc is a C port of dwmblocksr, itself a port of
[dwmblocks](https://github.com/torrinfail/dwmblocks) to Rust, in the same
way dwmc is a C port of dwmr: a function-for-function replica with the same
behaviour, including this fork's additions (asynchronous commands, clickable
blocks with `BLOCK_BUTTON`, UTF-8-safe truncation and clearing the status on
exit). Like dwmblocks, and unlike dwmblocksr, the blocks are compiled in
from `blocks.h`.

It is a drop-in replacement for dwmblocks and dwmblocksr: same status text,
same signal bytes before the blocks, same click protocol (dwm sends
SIGRTMIN+n with the button as the `sigqueue` value), and the same
`BLOCK_BUTTON` for the scripts.

## Requirements

- a C99 compiler and make
- Xlib headers (Debian: `libx11-dev`)

## Installation

    make                    # builds with ~/.config/dwmblocksc/blocks.h, or blocks.def.h
    sudo make install       # /usr/local/bin/dwmblocksc and the man page
    make install-config     # copy the shipped config/blocks.h to ~/.config/dwmblocksc/blocks.h

What dwmblocks' `compile.sh` did at build time, using the `sb-internet` block
instead of `sb-battery` when there is no `/sys/class/power_supply/BAT*`,
dwmblocksc does at startup (the `battery` field below), so one blocks.h works
on every machine and a plain `make && sudo make install` is enough.

## Running

Start it before dwm/dwmc, e.g. in `~/.xinitrc` (dwmc's autostart starts it):

    dwmblocksc &

`dwmblocksc -d <delim>` overrides the delimiter, `dwmblocksc -p` writes the
status to stdout instead of the root window name, `dwmblocksc -v` prints the
version.

## Configuration

Edit `~/.config/dwmblocksc/blocks.h`, then `make && sudo make install` and
restart dwmblocksc. The Makefile copies it to `blocks.h` (or `blocks.def.h`
when it does not exist) and rebuilds when it changes. See `config/blocks.h`
for the commented default.

    static const Block blocks[] = {
        /*Icon*/     /*Command*/                         /*Interval*/ /*Signal*/ /*Battery*/
        {"^5^ ",     "~/.local/bin/statusbar/sb-battery",  5,           3,         HasBattery},
        {"^6^ ",     "~/.local/bin/statusbar/sb-clock",    5,           1},
    };
    static char delim[] = " ";         /* "" for none */
    static unsigned int delimLen = 5;  /* at most this many bytes of delim are used */
    #define ASYNC 1                    /* 0 runs the commands one at a time */

- Commands run with `/bin/sh -c`, so `~` and shell syntax work. Only the
  first line of the output is used, cut to fit in 100 bytes per block
  (signal byte, icon and delimiter included).
- The interval is in seconds; 0 means the block only updates on its signal.
- Signal n updates the block on SIGRTMIN+n (`pkill -RTMIN+n dwmblocksc`)
  and makes it clickable; 0 means no signal. It has to be at most 30.
- `HasBattery` keeps the block only when there is a battery
  (`/sys/class/power_supply/BAT*`), `NoBattery` only when there is none;
  leave it out to always show the block. It is checked at startup.
- Status2d codes (`^2^`, `^c#rrggbb^`, `^B^`...`^N^`) in icons and output
  are passed to dwm unchanged.
- What dwmblocksr rejects when it loads its config (a signal above 30, an
  icon or `delimLen` too long for a block) makes dwmblocksc print the
  problem and exit 1 at startup.

## Scripts

A click runs the block's command with `BLOCK_BUTTON=<button>` and then
updates the block. Scripts that signal the status bar themselves have to use
the process name, e.g. `pkill -RTMIN+5 -x 'dwmblocks[rc]?'`, which matches
dwmblocks, dwmblocksr and dwmblocksc.

## Development

`make test` runs the unit tests under ASan and UBSan, against `blocks.def.h`
and `config/blocks.h`, each with `ASYNC` 1 and 0. `make debug` builds the
sanitized `dwmblocksc-debug` for testing on Xvfb.
