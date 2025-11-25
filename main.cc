#include <stdio.h>
#include <string>

#include "./LzssCompression.class.h"

int main(void){
	printf("Testing Compression.\n");
	LZSSCompression lzss;
	std::string msg = "this is my compression algorithm. It is very cool. I like compression.";
	printf("Compressing (%ld) %s\n", msg.length(), msg.c_str());
	if(!lzss.compress((char *)msg.c_str(), msg.length())){
		printf("Compression Failed: '%s'\n", lzss.getErrorMsg().c_str());
		exit(EXIT_FAILURE);
	}
	printf("Compression Succeeded.\n");
	printf("Result (%ld): ", lzss.out_s);
	for(int i=0; i<lzss.out_s; i++){
		printf("%c", lzss.out[i]);
	}printf("\n");
	exit(EXIT_SUCCESS);
}
