VERSION=1.0.0

all: extract-drs-windows-amd64-${VERSION}.exe extract-drs-linux-amd64-${VERSION}

extract-drs-windows-amd64-${VERSION}.exe: main.c
	x86_64-w64-mingw32-gcc -static -Os -o $@ $^
	x86_64-w64-mingw32-strip -s $@

extract-drs-linux-amd64-${VERSION}: main.c
	${HOME}/x-tools/x86_64-pc-linux-musl/bin/x86_64-pc-linux-musl-gcc -static -Os -o $@ $^
	${HOME}/x-tools/x86_64-pc-linux-musl/bin/x86_64-pc-linux-musl-strip -s $@

.PHONY: all
