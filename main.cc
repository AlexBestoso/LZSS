#include <stdio.h>
#include <string>

#include "./LzssCompression.class.h"

int main(void){
	printf("Testing Compression.\n");
	LZSSCompression lzss;
	char test[256];
	printf("Compressing (%d)\n\t", 256);
	for(int i=0; i<256; i++){
		test[i] = (char)i;
		printf("%d ", test[i]);
	}printf("\n");
	if(!lzss.compress(test, 256)){
		printf("Compression Failed: '%s'\n", lzss.getErrorMsg().c_str());
		exit(EXIT_FAILURE);
	}
	printf("Compression Succeeded, compressed to %ld bytes\n\n", lzss.out_s);
	for(int i=0; i<lzss.out_s; i++){
		printf("%d ", lzss.out[i]);
	}printf("\n");

	size_t compressedData_s = lzss.out_s;
	char *compressedData = new char[compressedData_s];
	for(int i=0; i<compressedData_s; i++)
		compressedData[i] = lzss.out[i];

	printf("\nTesting Decompression...\n");
	if(!lzss.decompress(compressedData, compressedData_s)){
		printf("Decompression Failed: '%s'\n", lzss.getErrorMsg().c_str());
		exit(EXIT_FAILURE);
	}

	printf("Decompression Succeeded, decompressed to %ld bytes\n\n", lzss.out_s);
	for(int i=0; i<lzss.out_s; i++){
		printf("%d ", lzss.out[i]);
	}printf("\n");
	
	delete[] compressedData;
	exit(EXIT_SUCCESS);
}
