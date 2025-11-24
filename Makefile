all:
	sed -i 's/^#define LZSSCOMPRESSION_VERBOSE_DEBUG .*$$/#define LZSSCOMPRESSION_VERBOSE_DEBUG 0/g' ./LzssCompression.class.h
	g++ main.cc -o main.o
clean:
	rm ./ main.o
debug:
	sed -i 's/^#define LZSSCOMPRESSION_VERBOSE_DEBUG .*$$/#define LZSSCOMPRESSION_VERBOSE_DEBUG 1/g' ./LzssCompression.class.h
	g++ main.cc -g -o main.o
