# LZSS - C++
LZSS Compression

<b>NOTICE : The JavaScript implementation has yet to go through any quality assurance testing. Though it should work becuase I said so. </b>

All known 8-bit bytes can be processed by this implementation.
This hasn't been tested with massive files, although it should work because of how it's designed.

Not all strings can be compressed. If there's no matches, this will produce a larger result than what was provided.

As of now, the maximum number of bytes that can be compressed is 256. The algorithm can be improved to enable larger matches.

This is not the most efficient implementation of LZSS compression. The `movDictionary()` allowcates a second buffer to shift the bytes, and
the string matching algorithm can likely be improved as well.

Other than that, this implementation uses bitwise operations to pack the compressed data. The method used involves providing a bit value of 0 before every series of 8 bits. These are your literals.
A bit of 1 is added to designate a "token", and is always followed by a series of 16 bits, a size and a distance, in that order.

To compile this, run `make`
