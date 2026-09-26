/* See LICENSE file for copyright and license details.
 *
 * dwmblocksc runs a command for every block of the status bar, puts their
 * first output lines together and sets the result as the name of the root
 * window, which dwm draws as its status text. Every block is updated every
 * `interval` seconds and when its signal SIGRTMIN+`signal` arrives.
 *
 * A block with a signal starts with that signal as a byte (< ' '), so dwm
 * can tell which block was clicked. dwm then sends the same signal with the
 * mouse button as sigqueue()'s value, and the block's command is run with
 * BLOCK_BUTTON=<button> in its environment.
 *
 * Signals are only written to a pipe by the signal handlers; statusloop()
 * poll()s that pipe, plus the output of every running command in async mode.
 *
 * To understand everything else, start reading statusloop().
 */
/* Mirrors dwmblocksr/src/dwmblocks.rs, dwmblocksr/src/main.rs and the Block
 * type, hasbattery() and selectblocks() of dwmblocksr/src/config.rs */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		100
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

/* Block.battery: like compile.sh's sb-battery swap, decided at startup */
enum { AnyBattery, HasBattery, NoBattery }; /* always, only with, only without a battery */

typedef struct {
	char *icon;
	char *command;
	unsigned int interval;
	unsigned int signal;
	int battery;
} Block;
/* a queued signal: the block signal and the clicked button, 0 for an update */
typedef struct {
	unsigned int signal;
	int button;
} SigEvent;

static void dummysighandler(int signum);
static void sighandler(int signum, siginfo_t *si, void *ucontext);
static void buttonhandler(const Block *block, int button);
static void getcmd(unsigned int i);
static void setblockstatus(const Block *block, char *output, const char *cmdout);
static void getcmds(int time);
static void getsigcmds(unsigned int signal);
static void setupsignals(void);
static int getstatus(char *str, char *last);
static void statusloop(void);
static void termhandler(int signum);
static void pstdout(void);
static void setroot(void);
static int setupX(void);
static int hasbattery(void);
static void selectblocks(int battery);

static void (*writestatus) (void) = setroot;
static Display *dpy;
static int screen;
static Window root;

#include "blocks.h"

/* dwmblocksr's parse() rejects a signal byte, icon and delimiter that do not
 * fit in CMDLENGTH, and signals above SIGRTMAX-SIGRTMIN or 31. blocks[] and
 * delimLen are not constant expressions in C and SIGRTMAX is a libc call, so
 * main() checks them at startup, before anything runs. */
#ifndef ASYNC
#define ASYNC 1
#endif

static const Block *selblocks[LENGTH(blocks)]; /* the blocks for this machine */
static unsigned int nblocks;
static char statusbar[LENGTH(blocks)][CMDLENGTH];
static char statusstr[2][STATUSLENGTH];
static volatile sig_atomic_t statusContinue = 1;
static int sigpipe[2] = { -1, -1 };
static int sigplus; /* SIGRTMIN, a libc call, so it is read once in setupsignals() */
static char *delimiter = delim;//delim from blocks.h, or the -d argument
#if ASYNC
typedef struct {
	int fd;//read end of the running command's output, -1 if not running
	int rerun;//the block was signalled again while its command was running
	size_t len;
	char out[CMDLENGTH];
} BlockCmd;
static BlockCmd blockcmds[LENGTH(blocks)];
static void readcmd(unsigned int i);
#endif

//builds the status of a block from the output of its command
void
setblockstatus(const Block *block, char *output, const char *cmdout)
{
	char tempstatus[CMDLENGTH];
	int start = 0, i, j, limit;
	size_t n;

	//mark the block with its signal so dwm can tell which block was clicked,
	//not when printing to stdout (-p) where the raw bytes would end up in the output
	if (block->signal && writestatus != pstdout)
		tempstatus[start++] = block->signal;
	n = MIN(strlen(block->icon), (size_t)(CMDLENGTH - 1 - start));
	memcpy(tempstatus + start, block->icon, n);
	i = start + n;
	//only use the first line of the output, as much of it as fits
	limit = delimLen + 1 < CMDLENGTH ? CMDLENGTH - (int)delimLen - 1 : 0;
	while (*cmdout && *cmdout != '\n' && i < limit)
		tempstatus[i++] = *cmdout++;
	//drop a UTF-8 character that was cut in half because the output was too long
	j = i;
	while (j > start && ((unsigned char)tempstatus[j-1] & 0xC0) == 0x80)
		j--;
	if (j > start) {
		unsigned char lead = tempstatus[j-1];
		int charlen = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
		if (i-(j-1) < charlen)
			i = j-1;
	}
	//leave the block out if block and command output are both empty
	if (i == start)
		i = 0;
	else if (delimiter[0] != '\0') {
		n = MIN(MIN(strlen(delimiter), delimLen), (size_t)(CMDLENGTH - 1 - i));
		memcpy(tempstatus + i, delimiter, n);
		i += n;
	}
	tempstatus[i] = '\0';
	memcpy(output, tempstatus, i + 1);
}

#if ASYNC
//starts the command of a block in the background, readcmd updates the block
//once the command is done
void
getcmd(unsigned int i)
{
	BlockCmd *cmd;
	int fds[2];
	pid_t pid;

	if (i >= nblocks)
		return;
	cmd = &blockcmds[i];
	if (cmd->fd != -1) {
		cmd->rerun = 1;
		return;
	}
	if (pipe(fds) == -1)
		return;
	pid = fork();
	if (pid == -1) {
		close(fds[0]);
		close(fds[1]);
		return;
	}
	if (pid == 0) {
		dup2(fds[1], STDOUT_FILENO);
		close(fds[0]);
		close(fds[1]);
		execl("/bin/sh", "sh", "-c", selblocks[i]->command, (char *)NULL);
		_exit(127);
	}
	close(fds[1]);
	fcntl(fds[0], F_SETFD, FD_CLOEXEC);
	fcntl(fds[0], F_SETFL, O_NONBLOCK);
	cmd->fd = fds[0];
	cmd->rerun = 0;
	cmd->len = 0;
}

//reads the output of the running command of block i and updates the block
//when the command is done
void
readcmd(unsigned int i)
{
	BlockCmd *cmd;
	char buf[256];
	ssize_t n;

	if (i >= nblocks)
		return;
	cmd = &blockcmds[i];
	//keep what fits, the rest is only read so the command can't block on a full pipe
	while ((n = read(cmd->fd, buf, sizeof(buf))) > 0) {
		size_t keep = MIN((size_t)n, sizeof(cmd->out)-1-cmd->len);
		memcpy(cmd->out+cmd->len, buf, keep);
		cmd->len += keep;
	}
	if (n == -1 && (errno == EAGAIN || errno == EINTR))
		return;
	close(cmd->fd);
	cmd->fd = -1;
	cmd->out[cmd->len] = '\0';
	setblockstatus(selblocks[i], statusbar[i], cmd->out);
	if (cmd->rerun)
		getcmd(i);
}
#else
//opens process *cmd and stores output in *output
void
getcmd(unsigned int i)
{
	//make sure status is same until output is ready
	char cmdout[CMDLENGTH] = {0};
	FILE *cmdf;

	if (i >= nblocks || !(cmdf = popen(selblocks[i]->command, "r")))
		return;
	if (!fgets(cmdout, sizeof(cmdout), cmdf))
		cmdout[0] = '\0';
	pclose(cmdf);
	setblockstatus(selblocks[i], statusbar[i], cmdout);
}
#endif

void
getcmds(int time)
{
	const Block *current;
	unsigned int i;

	for (i = 0; i < nblocks; i++) {
		current = selblocks[i];
#if ASYNC
		//let a command that takes longer than its interval finish first
		if (blockcmds[i].fd != -1)
			continue;
#endif
		if ((current->interval != 0 && (unsigned int)time % current->interval == 0) || time == -1)
			getcmd(i);
	}
}

//runs the command of a clicked block in the background with BLOCK_BUTTON set,
//then signals dwmblocksc to update the block from the command's normal output
void
buttonhandler(const Block *block, int button)
{
	extern char **environ;
	char shcmd[1024], btn[32], **envp, **e;
	size_t n = 0;
	pid_t child;

	if (snprintf(shcmd, sizeof(shcmd), "%s\nkill -%d %d", block->command,
	             sigplus + (int)block->signal, (int)getpid()) >= (int)sizeof(shcmd))
		return;
	snprintf(btn, sizeof(btn), "BLOCK_BUTTON=%d", button);
	/* setenv() is not async-signal-safe, so the command's environment
	 * (ours with BLOCK_BUTTON set) is built before forking */
	for (e = environ; *e; e++)
		n++;
	if (!(envp = calloc(n + 2, sizeof(char *))))
		return;
	for (n = 0, e = environ; *e; e++)
		if (strncmp(*e, "BLOCK_BUTTON=", 13))
			envp[n++] = *e;
	envp[n] = btn;
	//fork twice so the command is reparented to init and never left as a zombie
	child = fork();
	if (child == 0) {
		if (fork() == 0) {
			int devnull = open("/dev/null", O_WRONLY);
			if (devnull != -1)
				dup2(devnull, STDOUT_FILENO);
			setsid();
			execle("/bin/sh", "sh", "-c", shcmd, (char *)NULL, envp);
			_exit(127);
		}
		_exit(0);
	}
	if (child > 0)
		while (waitpid(child, NULL, 0) == -1 && errno == EINTR);
	free(envp);
}

void
getsigcmds(unsigned int signal)
{
	unsigned int i;

	for (i = 0; i < nblocks; i++)
		if (selblocks[i]->signal == signal)
			getcmd(i);
}

void
setupsignals(void)
{
	struct sigaction sa = { .sa_sigaction = sighandler, .sa_flags = SA_SIGINFO | SA_RESTART };
	unsigned int i;
	int s;

	//signals are queued on a pipe and handled in statusloop
	if (pipe(sigpipe) == -1) {
		perror("dwmblocksc: pipe");
		exit(1);
	}
	for (i = 0; i < 2; i++) {
		fcntl(sigpipe[i], F_SETFD, FD_CLOEXEC);
		fcntl(sigpipe[i], F_SETFL, O_NONBLOCK);
	}
	sigplus = SIGRTMIN;

	/* initialize all real time signals with dummy handler */
	for (s = SIGRTMIN; s <= SIGRTMAX; s++)
		signal(s, dummysighandler);

	sigemptyset(&sa.sa_mask);
	for (i = 0; i < nblocks; i++)
		if (selblocks[i]->signal > 0)
			sigaction(sigplus + selblocks[i]->signal, &sa, NULL);
}

/* put the blocks together in str, the previous status is copied to last;
 * nonzero if they differ */
int
getstatus(char *str, char *last)
{
	size_t len = 0, n;
	unsigned int i;

	memcpy(last, str, strlen(str) + 1);
	for (i = 0; i < nblocks; i++) {
		n = strlen(statusbar[i]);
		memcpy(str + len, statusbar[i], n);
		len += n;
	}
	n = strlen(delimiter);
	if (len >= n)
		len -= n;
	str[len] = '\0';
	return strcmp(str, last);//0 if they are the same
}

void
setroot(void)
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only set root if text has changed.
		return;
	XStoreName(dpy, root, statusstr[0]);
	XFlush(dpy);
}

int
setupX(void)
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "dwmblocksc: Failed to open display\n");
		return 0;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	return 1;
}

void
pstdout(void)
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only write out if text has changed.
		return;
	printf("%s\n", statusstr[0]);
	fflush(stdout);
}

void
statusloop(void)
{
	//the signal pipe, plus the output of every running command in async mode
	struct pollfd pfds[LENGTH(blocks)+1] = { { .fd = sigpipe[0], .events = POLLIN } };
	struct timespec now, next;
	unsigned int i = 0, j;
	int nfds, timeout;
	time_t ms;
	SigEvent ev;
#if ASYNC
	unsigned int running[LENGTH(blocks)+1];

	for (j = 0; j < LENGTH(blocks); j++)
		blockcmds[j].fd = -1;
#endif

	getcmds(-1);
	writestatus();
	clock_gettime(CLOCK_MONOTONIC, &next);
	next.tv_sec++;
	while (statusContinue) {
#if ASYNC
		//reap finished commands
		while (waitpid(-1, NULL, WNOHANG) > 0);
#endif
		clock_gettime(CLOCK_MONOTONIC, &now);
		ms = (next.tv_sec - now.tv_sec) * 1000 + (next.tv_nsec - now.tv_nsec) / 1000000;
		if (ms <= 0) {
			getcmds((int)++i);
			writestatus();
			clock_gettime(CLOCK_MONOTONIC, &next);
			next.tv_sec++;
			continue;
		}
		timeout = MIN(ms, INT_MAX);
		//wait until the next second, handling signals and output as they come in
		nfds = 1;
#if ASYNC
		for (j = 0; j < nblocks; j++) {
			if (blockcmds[j].fd != -1) {
				pfds[nfds] = (struct pollfd){ .fd = blockcmds[j].fd, .events = POLLIN };
				running[nfds++] = j;
			}
		}
#endif
		if (poll(pfds, nfds, timeout) <= 0)
			continue;
		if (pfds[0].revents) {
			while (read(sigpipe[0], &ev, sizeof(ev)) == sizeof(ev)) {
				if (!ev.signal)
					continue;//wakeup from termhandler
				if (ev.button) {
					for (j = 0; j < nblocks; j++)
						if (selblocks[j]->signal == ev.signal)
							buttonhandler(selblocks[j], ev.button);
				} else
					getsigcmds(ev.signal);
			}
		}
#if ASYNC
		for (j = 1; j < (unsigned int)nfds; j++)
			if (pfds[j].revents)
				readcmd(running[j]);
#endif
		writestatus();
	}
}

/* this signal handler should do nothing */
void
dummysighandler(int signum)
{
	return;
}

void
sighandler(int signum, siginfo_t *si, void *ucontext)
{
	//running the commands here isn't async-signal-safe, so just queue the signal.
	//dwm sends the clicked mouse button with sigqueue, a plain kill means update.
	int olderrno = errno;
	SigEvent ev = { signum - sigplus, si && si->si_code == SI_QUEUE ? si->si_value.sival_int : 0 };

	write(sigpipe[1], &ev, sizeof(ev));
	errno = olderrno;
}

void
termhandler(int signum)
{
	//also wake up statusloop, in case the signal came just before it started waiting
	int olderrno = errno;
	SigEvent ev = { 0, 0 };

	statusContinue = 0;
	write(sigpipe[1], &ev, sizeof(ev));
	errno = olderrno;
}

/* compile.sh's test: is there a /sys/class/power_supply/BAT?* entry? */
int
hasbattery(void)
{
	DIR *dir;
	struct dirent *d;
	int found = 0;

	if (!(dir = opendir("/sys/class/power_supply")))
		return 0;
	while (!found && (d = readdir(dir)))
		found = !strncmp(d->d_name, "BAT", 3) && d->d_name[3] != '\0';
	closedir(dir);
	return found;
}

/* keep the blocks that are for this machine: HasBattery blocks only with a
 * battery, NoBattery blocks only without one */
void
selectblocks(int battery)
{
	unsigned int i;

	for (nblocks = 0, i = 0; i < LENGTH(blocks); i++)
		if (blocks[i].battery == AnyBattery || (blocks[i].battery == HasBattery) == !!battery)
			selblocks[nblocks++] = &blocks[i];
}

int
main(int argc, char *argv[])
{
	unsigned int i;
	int sigmax;

	for (i = 1; i < (unsigned int)argc; i++) {//Handle command line arguments
		if (!strcmp("-d", argv[i]) && i + 1 < (unsigned int)argc)
			delimiter = argv[++i];
		else if (!strcmp("-p", argv[i]))
			writestatus = pstdout;
		else if (!strcmp("-v", argv[i])) {
			fputs("dwmblocksc-"VERSION"\n", stderr);
			return 1;
		}
	}
	/* the signal byte, the icon and the delimiter with its NUL must fit in
	 * a block's CMDLENGTH bytes; a block's signal is also the byte dwm reads
	 * to tell the blocks apart, so it has to be a control character (< ' ') */
	if (delimLen > CMDLENGTH - 3) {
		fprintf(stderr, "dwmblocksc: blocks.h: delimLen: must be at most %d\n", CMDLENGTH - 3);
		return 1;
	}
	sigmax = SIGRTMAX - SIGRTMIN;
	sigmax = sigmax < 0 ? 0 : MIN(sigmax, 31);
	for (i = 0; i < LENGTH(blocks); i++) {
		if (1 + strlen(blocks[i].icon) + delimLen + 1 > CMDLENGTH) {
			fprintf(stderr, "dwmblocksc: blocks.h: blocks[%u]: icon is %zu bytes, at most %u fit with delimLen %u\n",
			        i, strlen(blocks[i].icon), CMDLENGTH - 2 - delimLen, delimLen);
			return 1;
		}
		if (blocks[i].signal > (unsigned int)sigmax) {
			fprintf(stderr, "dwmblocksc: blocks.h: blocks[%u]: signal %u is out of range (0..=%d)\n",
			        i, blocks[i].signal, sigmax);
			return 1;
		}
	}
	selectblocks(hasbattery());
	delimLen = MIN(delimLen, strlen(delimiter));
	delimiter[delimLen++] = '\0';
	if (!setupX())
		return 1;
	setupsignals();
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
	//clear the status so dwm doesn't keep showing stale blocks (e.g. a clock that
	//stopped), an empty status makes dwm fall back to its "dwm-<version>" text
	if (writestatus == setroot)
		XStoreName(dpy, root, "");
	XCloseDisplay(dpy);
	return 0;
}
