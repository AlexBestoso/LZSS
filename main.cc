#include <stdio.h>
#include <string>

#include "./LzssCompression.class.h"

int main(void){
	printf("Testing Compression.\n");
	LZSSCompression lzss;
	std::string msg = "this is my compression algorithm. It is very cool. I like compression.";
	if(!lzss.compress((char *)msg.c_str(), msg.length())){
		printf("Compression Failed: '%s'\n", lzss.getErrorMsg().c_str());
		exit(EXIT_FAILURE);
	}else{
		printf("Compression Succeeded.\n");
	}
	exit(EXIT_SUCCESS);
}
