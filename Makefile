
# make install will install to:
BIN_DIR = /usr/bin

CFLAGS += "-lasound"

all: gta02-gsm-bt-fix

gta02-gsm-bt-fix: common.h

clean:
	rm -f gta02-gsm-bt-fix

install: all
	cp gta02-gsm-bt-fix ${BIN_DIR}

uninstall:
	rm -f ${BIN_DIR}/gta02-gsm-bt-fix

