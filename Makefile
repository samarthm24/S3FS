COMPILER = gcc
FILESYSTEM_FILES = newfs3.c 

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o ssfs `pkg-config fuse --cflags --libs` -DFUSE_USE_VERSION=25 -lm -g
	echo 'To Mount: ./ssfs -f [mount point]'

clean:
	rm fs
