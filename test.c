/* See LICENSE file for copyright and license details. */
/* Mirrors the #[cfg(test)] modules of dwmblocksr/src/dwmblocks.rs and
 * dwmblocksr/src/config.rs */
/* Not ported: the tests of the TOML config loader. That blocks.def.h and
 * config/blocks.h build, with ASYNC 1 and 0, is what make test checks by
 * building this against each. */
#include <assert.h>

#define main dwmblocksc_main
#include "dwmblocksc.c"
#undef main

static char delimbuf[CMDLENGTH];
static char out[CMDLENGTH];

/* the delimiter as main() leaves it: cut to delimLen bytes, delimLen counting the NUL */
static void
setdelim(const char *d)
{
	snprintf(delimbuf, sizeof delimbuf, "%s", d);
	delimiter = delimbuf;
	delimLen = MIN(5, strlen(delimiter));
	delimiter[delimLen++] = '\0';
}

static const char *
status(const char *icon, unsigned int signal, const char *cmdout)
{
	Block b = { (char *)icon, "", 0, signal, AnyBattery };

	setblockstatus(&b, out, cmdout);
	return out;
}

static void
signal_byte(void)
{
	setdelim(" ");
	writestatus = setroot;
	assert(!strcmp(status("^2^ ", 5, "abc\nsecond line"), "\x05^2^ abc "));
	assert(!strcmp(status("^2^ ", 0, "abc"), "^2^ abc "));
}

static void
pstdout_has_no_signal_byte(void)
{
	setdelim(" ");
	writestatus = pstdout;
	assert(!strcmp(status("^2^ ", 5, "abc\n"), "^2^ abc "));
	writestatus = setroot;
}

static void
utf8_cut(void)
{
	char o[CMDLENGTH + 8], want[CMDLENGTH + 8];

	setdelim(" ");
	/* 97 = CMDLENGTH - delimLen(2) - 1 bytes fit: 96 x and the first byte of é */
	memset(o, 'x', 96);
	strcpy(o + 96, "é");
	memset(want, 'x', 96);
	strcpy(want + 96, " ");
	assert(!strcmp(status("", 0, o), want));
	/* a 3-byte glyph cut after 2 bytes */
	memset(o, 'x', 95);
	strcpy(o + 95, "\xef\x80\x97");
	memset(want, 'x', 95);
	strcpy(want + 95, " ");
	assert(!strcmp(status("", 0, o), want));
	/* a whole one is kept */
	memset(o, 'x', 94);
	strcpy(o + 94, "\xef\x80\x97");
	memcpy(want, o, 97);
	strcpy(want + 97, " ");
	assert(!strcmp(status("", 0, o), want));
}

static void
status2d_codes_pass_through(void)
{
	setdelim(" ");
	assert(!strcmp(status("", 6, "^c#ff0000^^B^\xef\x80\x97^N^ 42%\n"),
	               "\x06^c#ff0000^^B^\xef\x80\x97^N^ 42% "));
}

static void
empty_block(void)
{
	setdelim(" ");
	assert(!strcmp(status("", 5, ""), ""));
	assert(!strcmp(status("", 5, "\nsecond"), ""));
	assert(!strcmp(status("", 0, ""), ""));
	assert(!strcmp(status("^4^", 0, ""), "^4^ "));
	/* output stops at a NUL, like the C string */
	assert(!strcmp(status("", 0, "ab\0cd"), "ab "));
}

static void
delimiter_cut(void)
{
	char o[200];

	setdelim("");
	assert(!strcmp(status("", 0, "abc"), "abc"));
	/* cut to delimLen (5) bytes */
	setdelim(" | ab | ");
	assert(!strcmp(status("", 0, "abc"), "abc | ab"));
	/* the output limit shrinks with the delimiter: 100 - 6 - 1 = 93 bytes */
	memset(o, 'x', sizeof o - 1);
	o[sizeof o - 1] = '\0';
	assert(strlen(status("", 0, o)) == 93 + 5);
}

static void
getstatus_joins_blocks(void)
{
	char s[2][STATUSLENGTH] = { "", "" };

	setdelim(" ");
	writestatus = pstdout;
	nblocks = 3;
	memset(statusbar, 0, sizeof statusbar);
	assert(!getstatus(s[0], s[1])); /* "" is what was there */
	strcpy(statusbar[0], status("", 0, "a"));
	strcpy(statusbar[2], status("", 0, "c"));
	assert(getstatus(s[0], s[1]));
	assert(!strcmp(s[0], "a c")); /* trailing delimiter removed */
	assert(!getstatus(s[0], s[1]));
	assert(!strcmp(s[0], "a c"));
	strcpy(statusbar[1], status("", 0, "b"));
	assert(getstatus(s[0], s[1]));
	assert(!strcmp(s[0], "a b c"));
	/* no delimiter: nothing removed */
	setdelim("");
	nblocks = 1;
	strcpy(statusbar[0], status("", 0, "ab"));
	assert(getstatus(s[0], s[1]));
	assert(!strcmp(s[0], "ab"));
	writestatus = setroot;
}

static int
endswith(const char *s, const char *suffix)
{
	size_t n = strlen(s), m = strlen(suffix);

	return n >= m && !strcmp(s + n - m, suffix);
}

static int
selected(const char *suffix)
{
	unsigned int i;

	for (i = 0; i < nblocks; i++)
		if (endswith(selblocks[i]->command, suffix))
			return 1;
	return 0;
}

/* The icons are the Nerd Font glyphs of blocks.h, byte for byte. */
static void
icons_match_blocks_h(void)
{
	assert(!strcmp(blocks[5].icon, "^2^\xef\x83\x82  "));
	assert(!strcmp(blocks[6].icon, "^3^ \xef\x8b\x88 "));
	assert(!strcmp(blocks[10].icon, "^6^ \xef\x80\x97 "));
}

/* compile.sh: sb-battery with a battery, sb-internet without one. */
static void
battery_blocks(void)
{
	selectblocks(1);
	assert(nblocks == 10);
	assert(selected("sb-battery") && !selected("sb-internet"));
	selectblocks(0);
	assert(nblocks == 10);
	assert(selected("sb-internet") && !selected("sb-battery"));
}

int
main(void)
{
	signal_byte();
	pstdout_has_no_signal_byte();
	utf8_cut();
	status2d_codes_pass_through();
	empty_block();
	delimiter_cut();
	getstatus_joins_blocks();
	icons_match_blocks_h();
	battery_blocks();
	printf("dwmblocksc: all tests passed (ASYNC %d)\n", ASYNC);
	return 0;
}
