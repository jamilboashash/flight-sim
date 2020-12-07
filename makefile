CFLAGS = -pthread -lm -Wall -pedantic -std=gnu99

rocsources = roc.c
controlsources = control.c airplane.c airplane.h
mappersources = mapper.c airport.c airport.h
sharedsources = linkedList.c linkedList.h mapperProtocol.c mapperProtocol.h

.PHONY: all clean debug test fixed
.DEFAULT: all

all: roc control mapper

roc: $(rocsources) $(sharedsources)
	gcc $(CFLAGS) $(rocsources) $(sharedsources) -o roc

control: $(controlsources) $(sharedsources)
	gcc $(CFLAGS) $(controlsources) $(sharedsources) -o control

mapper: $(mappersources) $(sharedsources)
	gcc $(CFLAGS) $(mappersources) $(sharedsources) -o mapper

clean:
	rm -rf ./testres* ./roc ./control ./mapper

debug: CFLAGS += -DDEBUG=1 -g
debug: all

test: clean
test: debug
test:
	testa4.sh

# non debug mode but fixed port no
fixed: CFLAGS += -DCONST_PORT=1
fixed: all
