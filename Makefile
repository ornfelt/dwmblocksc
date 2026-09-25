# dwmblocksc - modular status bar for dwm (C port of dwmblocksr)
# See LICENSE file for copyright and license details.

include config.mk

# blocks.h is a copy of ~/.config/dwmblocksc/blocks.h, or of blocks.def.h
# when there is none; under sudo, use the invoking user's blocks, not root's
USERHOME = ${HOME}
ifneq (${SUDO_USER},)
USERHOME = $(shell getent passwd ${SUDO_USER} | cut -d: -f6)
endif
CONFDIR = ${USERHOME}/.config/dwmblocksc

all: dwmblocksc

blocks.h: blocks.def.h $(wildcard ${CONFDIR}/blocks.h)
	if [ -e ${CONFDIR}/blocks.h ]; then cp ${CONFDIR}/blocks.h $@; else cp blocks.def.h $@; fi

dwmblocksc: dwmblocksc.c blocks.h config.mk
	${CC} ${CFLAGS} -o $@ dwmblocksc.c ${LDFLAGS}

# dwmblocksc with the sanitizers, for the functional tests
debug: dwmblocksc-debug

dwmblocksc-debug: dwmblocksc.c blocks.h config.mk
	${CC} ${CFLAGS} ${DEBUGCFLAGS} -o $@ dwmblocksc.c ${LDFLAGS}

# the unit tests against both configs, blocks.def.h and the shipped
# config/blocks.h (in test.d, whose blocks.h dwmblocksc.c then includes),
# each with ASYNC 1 and 0
test: test.c dwmblocksc.c blocks.def.h config/blocks.h config.mk
	mkdir -p test.d
	ln -sf ../test.c ../dwmblocksc.c test.d/
	for c in blocks.def.h config/blocks.h; do \
		for a in 1 0; do \
			cp $$c test.d/blocks.h && \
			${CC} ${CFLAGS} ${DEBUGCFLAGS} -DASYNC=$$a -o test.d/test test.d/test.c ${LDFLAGS} && \
			./test.d/test || exit 1; \
		done; \
	done

clean:
	rm -f dwmblocksc dwmblocksc-debug blocks.h
	rm -rf test.d

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwmblocksc ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwmblocksc
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwmblocksc.1 > ${DESTDIR}${MANPREFIX}/man1/dwmblocksc.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwmblocksc.1

# copy the shipped blocks to ~/.config/dwmblocksc unless some are already
# there; dwmblocksc itself picks sb-battery or sb-internet at startup (the
# blocks' battery field), which dwmblocks' compile.sh did at build time
install-config:
	mkdir -p ${CONFDIR}
	[ -e ${CONFDIR}/blocks.h ] || cp config/blocks.h ${CONFDIR}/blocks.h

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwmblocksc\
		${DESTDIR}${MANPREFIX}/man1/dwmblocksc.1

.PHONY: all debug test clean install install-config uninstall
