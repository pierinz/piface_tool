CC:=gcc
CFLAGS:=-O2 -pipe -Wall

prefix:=/usr/local

piface_tool: main.o pfio.o
	$(CC) $(CFLAGS) $^ -o $@

*.o: *.c
	$(CC) $(CFLAGS) $< -c -o $@

install: piface_tool
	mkdir -p $(prefix)/sbin/
	install -m 0755 piface_tool -t $(prefix)/sbin/ $<
.PHONY: install

uninstall:
	rm -f $(prefix)/sbin/piface_tool

clean:
	rm -f *.o
	rm -f piface_tool
.PHONY: clean
