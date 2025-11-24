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
	}
	printf("Compression Succeeded.\n");
	printf("Result : ");
	for(int i=0; i<lzss.out_s; i++){
		printf("%c", lzss.out[i]);
	}printf("\n");
	exit(EXIT_SUCCESS);
}
