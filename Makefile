SRC=main.c
TARGET=ucbdma

PREFIX=/usr/local/google/home/mitthu/dev/akaros/kern/kfs/bin
PREFIX_SSH=qemu:/bin
CC=x86_64-ucb-akaros-gcc

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

install:
	install $(TARGET) $(PREFIX)/
	-scp $(TARGET) $(PREFIX_SSH)/

clean:
	rm -rf $(TARGET)
