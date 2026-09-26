/* See LICENSE file for copyright and license details. */
/* Mirrors Config::default() in dwmblocksr/src/config.rs */
//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
    /*Icon*/        /*Command*/                                     /*Update Interval*/     /*Update Signal*/   /*Battery*/
    /* {"^c1^",        "~/.local/bin/my_scripts/spotify_dwmblocks.sh", 5,                      12}, */
    {"",            "~/.local/bin/my_scripts/spotify_dwmblocks.sh", 5,                      12},
    {"",            "~/.local/bin/statusbar/sb-claude",             30,                     6},
    /* net down/up, memory and cpu, shown/hidden with mod-ctrl-p (sb-sysinfo toggle), a click shows details */
    {"",            "~/.local/bin/statusbar/sb-sysinfo net",        2,                      13},
    {"",            "~/.local/bin/statusbar/sb-sysinfo mem",        2,                      14},
    {"",            "~/.local/bin/statusbar/sb-sysinfo cpu",        2,                      15},
    {"^2^  ",      "~/.local/bin/statusbar/weather",               1800,                   5},
    {"^3^  ",      "~/.local/bin/statusbar/cputemp",               5,                      4},
    {"^4^ ",        "~/.local/bin/statusbar/sb-volume",             0,                      10},
    /* without a battery, sb-internet takes sb-battery's place */
    {"^5^ ",        "~/.local/bin/statusbar/sb-internet",           5,                      3,                  NoBattery},
    {"^5^ ",        "~/.local/bin/statusbar/sb-battery",            5,                      3,                  HasBattery},
    {"^6^  ",      "~/.local/bin/statusbar/sb-clock",              5,                      1},
};

//sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char delim[] = " ";
//at most this many bytes of the delimiter are used
static unsigned int delimLen = 5;

//1 runs the block commands in parallel, so a slow command (e.g. one waiting on
//the network) can't hold up the other blocks, signals and clicks. 0 runs them
//one at a time like upstream dwmblocks.
#ifndef ASYNC
#define ASYNC 1
#endif
