CFLAGS ?= -O2 -Wall -Wextra -Werror

all: build/watch

build/watch: src/watch.c
	mkdir -p build
	$(CC) $(CFLAGS) $< -o $@

check: all
	python3 -m unittest discover -s ../tests -v

clean:
	rm -rf build

.PHONY: all check clean
