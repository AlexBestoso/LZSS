#JavaScript Implementation
This is a JavaScript implementation of the LZSS compression algorithm.
This may not fit all your needs, and hasn't been tested against other known implementations of the algorithm.
To use this class, please note the following:

The `compress()` and `decompress()` functions expect an array of data, where the values in the array are all integers.
Meaning that if you want to compress a string, you must `split()` that string into an array, and then use `charCodeAt()` before providing the data to the `compress()` function.
To decompress, you may need to convert an integer string into a true int via `parseInt()`. And if you want the result back as a char, you will need to use the `String.fromCharCode()` function on each outputted value.
