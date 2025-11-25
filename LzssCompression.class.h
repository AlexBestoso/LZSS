#define LZSSCOMPRESSION_VERBOSE_DEBUG 1
class LZSSCompression{
	private:
		
		signed int *dictionary;
		size_t dictionary_s;
		
		signed int *lookahead;
		size_t lookahead_s;

		size_t matchMinimum;

		signed int error;
		int compressedCount;

		size_t outBitCount;
		int overflowOffset;
		
		void destroyOut(void){
			if(this->out != NULL){
				delete[] this->out;
				this->out = NULL;
			}
			this->out_s = 0;
			this->outBitCount = 0;
		}
		void destroyDictionary(void){
			if(this->dictionary != NULL){
				delete[] this->dictionary;
				this->dictionary = NULL;
			}
			this->dictionary_s = 0;
		}
		void destroyLookahead(void){
			if(this->lookahead != NULL){
				delete[] this->lookahead;
				this->lookahead = NULL;
			}
			this->lookahead_s = 0;
		}

		void init(void){
			this->error = -1;
			this->out = NULL;
			this->out_s = 0;
			this->dictionary = NULL;
			this->dictionary_s = 0;
			this->lookahead = NULL;
			this->lookahead_s = 0;
			this->matchMinimum = 5;
			this->compressedCount = 0;
			this->outBitCount = 0;
			this->overflowOffset = 0;
		}


		bool movDictionary(signed int val){
			signed int *transfer = new signed int[this->dictionary_s-1];
			for(int i=0; i<this->dictionary_s; i++)
				transfer[i] = this->dictionary[i];
			for(int i=1; i<this->dictionary_s; i++){
				this->dictionary[i] = transfer[i-1];
			}
			this->dictionary[0] = val;
			delete[] transfer;
			return true;
		}

		void printDictionary(void){
			if(this->dictionary == NULL) return;
			printf("dictionary : ");
			for(int i=0; i<this->dictionary_s; i++){
				printf("%c", (char)this->dictionary[i]);
			}printf("\n");
		}
	
		bool shiftLookahead(char *data, size_t dataSize, int index){
			if(data == NULL) return false;
			for(int i=1; i<this->lookahead_s; i++){
				this->lookahead[i-1] = this->lookahead[i];
			}
			if(index >= dataSize || index < 0){
				this->lookahead[this->lookahead_s-1] = -1;
			}else{
				this->lookahead[this->lookahead_s-1] = (signed int)data[index];
			}
			return true;
		}
		void printLookahead(void){
			if(this->lookahead == NULL) return;
			printf("lookahead : ");
			for(int i=0; i<this->lookahead_s; i++)
				printf("%c", (char)this->lookahead[i]);
			printf("\n");
		}
		
		// We may need to add one to the token offset if we want to be compatible with the standard. 
		// this function is depricated.
		std::string dictToLookCheck(void){
			this->compressedCount = 0;
			if(this->dictionary == NULL){
				return "";
			}
			if(this->lookahead == NULL){
				return "";
			}
			std::string ret = "";
			
			size_t possibleMatchesSize = 0;
			size_t possibleSize = 0;
			int *possibleMatches = NULL;
			int *transferBuffer = NULL;
			int trueMatch=-1;
			
			for(int l=0; l<this->lookahead_s; l++){
				signed int target = this->lookahead[l];
				if(target == -1) break; // no more data to process.
				if(possibleSize == 0 && l > 0) break; // no matches found

				if(possibleSize == 0){
					for(int d=0; d<this->dictionary_s; d++){
						if(this->dictionary[d] == -1) break;
						if(target == this->dictionary[d]){
							possibleMatchesSize++;
							if(possibleMatches == NULL){
								possibleMatches = new int[possibleMatchesSize];
								possibleMatches[possibleMatchesSize-1] = d;
							}else{
								transferBuffer = new int[possibleMatchesSize-1];
								for(int i=0; i<possibleMatchesSize-1; i++)
									transferBuffer[i] = possibleMatches[i];
								delete[] possibleMatches;
								possibleMatches = NULL;
								possibleMatches = new int[possibleMatchesSize];
								for(int i=0; i<possibleMatchesSize-1; i++)
									possibleMatches[i] = transferBuffer[i];
								possibleMatches[possibleMatchesSize-1] = d;
								delete[] transferBuffer;
								transferBuffer = NULL;
							}
							
						}
					}
					if(possibleMatchesSize > 0)
						possibleSize++;
				}else{
					transferBuffer = new int[possibleMatchesSize];
					size_t transferCount=0;
					for(int p=0; p<possibleMatchesSize; p++){
						int dIndex = possibleMatches[p]-possibleSize;
						if(dIndex < 0){
							continue; // only a size of 1 is possible here.
						}
						unsigned int dictVal = this->dictionary[dIndex];
						if(target == -1 || dictVal == -1)
							break;
						if(target == dictVal){
							transferBuffer[transferCount] = possibleMatches[p];
							transferCount++;
						}
						
					}
					if(transferCount > 0){
						possibleSize++;
						delete[] possibleMatches;
						possibleMatches = NULL;
						possibleMatchesSize = transferCount;
						possibleMatches = new int[possibleMatchesSize];
						for(int i=0; i<transferCount; i++){
							possibleMatches[i] = transferBuffer[i];
						}
					}
					delete[] transferBuffer;
					transferBuffer = NULL;
					if(transferCount <= 0) break;
				}
			}
			
			if(possibleMatches != NULL && possibleSize > 0)
				ret = "<"+std::to_string(possibleSize)+","+std::to_string(possibleMatches[0])+">";
			int returnSize = ret.length();
			if(possibleSize <= 0){
				this->compressedCount = 1;
				ret = "";
				if(this->lookahead[0] == -1) return "";
				ret += this->lookahead[0];
				this->movDictionary(lookahead[0]);
			}else if(possibleSize <= returnSize){
				ret = "";
				for(int i=0; i<possibleSize; i++){
					if(this->lookahead[i] == -1) break;
					ret += this->lookahead[i];
					this->movDictionary(this->lookahead[i]);
				}
				this->compressedCount = possibleSize;
			}else{
				for(int i=0; i<possibleSize; i++){
					if(this->lookahead[i] == -1) break;
					this->movDictionary(this->lookahead[i]);
				}
				this->compressedCount = possibleSize;
			}

			
			return ret;
		}

		bool resizeOutput(size_t newSize){
			if(this->out == NULL) return false;
			if(this->out_s <= 0) return false;
			
			size_t oldSize = this->out_s;
			char *oldBuff = new char[oldSize];
			for(int i=0; i<this->out_s; i++)
				oldBuff[i] = this->out[i];
			size_t bitcount = this->outBitCount;

			this->destroyOut();
			this->out_s = newSize;
			this->out = new char[this->out_s];
			for(int i=0; i<oldSize; i++){
				this->out[i] = oldBuff[i];
			}
			for(int i=oldSize; i<newSize; i++)
				this->out[i] = 0x00;
			delete[] oldBuff;
			return true;
		}

		/*
		 * A binary 0 before a series of 8 bits means
		 * that the byte should be interpreted as a literal byte.
		 * this constructed bit stream will be injected into this->out.
		 * */
		bool insertLiteral(size_t count){
			if(this->lookahead == NULL){
				this->error = 0x200;
				return false;
			}
			if(this->lookahead_s <= 0){ 
				this->error = 0x201;
				return false;
			}
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("\033[35;1m[DBG] Inserting Literal\n");
				printf("\033[35;1m[DBG] Processing %ld bytes, starting out size is %ld, starting offset is %d\n", count, this->out_s, this->overflowOffset);
			#endif
			// modify this section so that if the offset if not a perfect byte, no new char is added.
			size_t newBits = (8*count)+count;
			int start = this->out_s <= 0 ? 0 : this->out_s-1;
			if(this->out == NULL || this->out_s <= 0){
				this->destroyOut();
				this->outBitCount = newBits;
				if((this->overflowOffset%8) == 0)
					this->out_s += count+1;
				else
					this->out_s += count;

				this->out = new char[this->out_s];
			}else{
				this->outBitCount += newBits;
				size_t newOutSize = this->out_s;
				if((this->overflowOffset%8) == 0){
					newOutSize += count+1;
				}else{
					newOutSize += count;
				}
				this->resizeOutput(newOutSize);
			}
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("[DBG] new out size is %ld, starting process on index %d\n", this->out_s, start);
			#endif
			int end = this->out_s;
			int lIterator = 0;
			for(int i=start; i<end && lIterator<this->lookahead_s && lIterator<count; i++){
				signed int lookchar = this->lookahead[lIterator];
				if(lookchar == -1) break;
				lIterator++;
				
				//         flag                         remove excess       shift to fit container
				out[i] += (0x00<<(7-this->overflowOffset)) + ((lookchar&0xff) >> (1+this->overflowOffset));
				#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("[DBG] Original : '0x%x' | flag : '0x%x' | right shifted byte (%d) : '0x%x'\n", lookchar, (0x00<<(7-this->overflowOffset)), 1+this->overflowOffset, ((lookchar&0xff) >> (1+this->overflowOffset)));
				#endif
					
				// ajust for the shifting offset position
				this->overflowOffset = (this->overflowOffset + 1) % 8;
					
				//  add remainder to next output char.
				i++;
				if(i<end && this->overflowOffset > 0){
					//            clear used bits
					int clearer = 0xff >> (8-this->overflowOffset);
					lookchar &= clearer;
					//            add unused bits
					out[i] += lookchar << (8-this->overflowOffset);
					#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
                                	printf("[DBG] cleared byte : '0x%x' | left shifted byte (%d) : '0x%x'\n", lookchar, 8-this->overflowOffset, lookchar << (8-this->overflowOffset));
                                	#endif
				}
				i--;			
				#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("[DBG] Generated output : ");
				for(int i=0; i<this->out_s; i++){
					printf("0x%x ", this->out[i]&0x000000ff );
				}printf("\n");
				#endif
			}
			printf("\033[0m");

			return true;
		}
		/*
		 * A binary 1 before a seires of 8 bits means that we're defining
		 * a token size, and that the next 8 bits are an offset/distance.
		 * this bit stream will be injected into this->out.
		 * */
		// TODO: make this size relative to dictionary size, maybe
		bool insertToken(int size, int distance){
			if(this->out == NULL){ 
				this->error = 0x202;
				return false;
			}
			if(this->out_s <= 0){
				this->error = 0x203;
				return false; // this shouldn't be running before insertLiteral
			}
			// we only support a max compression of 255...for now.
			if(size > 0xff){
				this->error = 0x204;
				return false;
			}
			if(distance > 0xff){
				this->error = 0x205;
				return false;
			}

			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
                                printf("\033[35;1m[DBG] Inserting Token\n");
                        #endif

			int newBits = 17; // 2 bytes, plus 1 flag
			int start = this->out_s <= 0 ? 0 : this->out_s;
			// re-allocate output buffer.
			this->outBitCount += newBits;
			size_t newOutSize = this->out_s;
			if((this->overflowOffset%8) != 0){
				newOutSize += 2+1;
			}else{
				newOutSize += 2;
			}
			this->resizeOutput(newOutSize);
			int end = this->out_s;

			// inject the size.
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("[DBG] size : 0x%x | token : 0x%x | right shifted value(%d) : 0x%x\n", size, (0x01<<(7-this->overflowOffset)), 1+this->overflowOffset, ((size&0xff) >> (1+this->overflowOffset)));
			#endif
			out[start]  += (0x01<<(7-this->overflowOffset)) + ((size&0xff) >> (1+this->overflowOffset));
			this->overflowOffset = (this->overflowOffset + 1) % 8;
			start++;
			if(start >= end) return false;
			int clearer = 0xff >> (8-this->overflowOffset);
                        size &= clearer;
                        out[start] = size << (8-this->overflowOffset);
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("[DBG] cleared size : '0x%x' | left shifted size (%d) : '0x%x'\n", size, 8-this->overflowOffset, size << (8-this->overflowOffset));
			printf("[DBG] Generated output : ");
			for(int i=0; i<this->out_s; i++){
				printf("0x%x ", this->out[i]&0x000000ff );
			}printf("\n");
			#endif			

			// inject the distance (doesn't change overflow, full byte.
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
                        printf("[DBG] distance : 0x%x | token : N/A | right shifted value(%d) : 0x%x\n", distance, 1+this->overflowOffset, ((distance&0xff) >> (1+this->overflowOffset)));
                        #endif
			out[start]  += ((distance&0xff) >> (this->overflowOffset));
			start++;
			if(start >= this->out_s) return false;
			clearer = 0xff >> (8-this->overflowOffset);
                        distance &= clearer;
                        out[start] = distance << (8-this->overflowOffset);
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("[DBG] cleared size : '0x%x' | left shifted size (%d) : '0x%x'\n", distance, 8-this->overflowOffset, distance << (8-this->overflowOffset));
			printf("[DBG] Generated output : ");
			for(int i=0; i<this->out_s; i++){
				printf("0x%x ", this->out[i]&0x000000ff );
			}printf("\n\033[0m");
			#endif			

			return true;	
		}

		bool encode(void){
			this->compressedCount = 0;
			if(this->dictionary == NULL){
				return false;
			}
			if(this->lookahead == NULL){
				return false;
			}
			size_t possibleMatchesSize = 0;
			size_t possibleSize = 0;
			int *possibleMatches = NULL;
			int *transferBuffer = NULL;
			
			for(int l=0; l<this->lookahead_s; l++){
				signed int target = this->lookahead[l];
				if(target == -1) break; // no more data to process.
				if(possibleSize == 0 && l > 0) break; // no matches found

				if(possibleSize == 0){
					for(int d=0; d<this->dictionary_s; d++){
						if(this->dictionary[d] == -1) break;
						if(target == this->dictionary[d]){
							possibleMatchesSize++;
							if(possibleMatches == NULL){
								possibleMatches = new int[possibleMatchesSize];
								possibleMatches[possibleMatchesSize-1] = d;
							}else{
								transferBuffer = new int[possibleMatchesSize-1];
								for(int i=0; i<possibleMatchesSize-1; i++)
									transferBuffer[i] = possibleMatches[i];
								delete[] possibleMatches;
								possibleMatches = NULL;
								possibleMatches = new int[possibleMatchesSize];
								for(int i=0; i<possibleMatchesSize-1; i++)
									possibleMatches[i] = transferBuffer[i];
								possibleMatches[possibleMatchesSize-1] = d;
								delete[] transferBuffer;
								transferBuffer = NULL;
							}
							
						}
					}
					if(possibleMatchesSize > 0)
						possibleSize++;
				}else{
					transferBuffer = new int[possibleMatchesSize];
					size_t transferCount=0;
					for(int p=0; p<possibleMatchesSize; p++){
						int dIndex = possibleMatches[p]-possibleSize;
						if(dIndex < 0){
							continue; // only a size of 1 is possible here.
						}
						unsigned int dictVal = this->dictionary[dIndex];
						if(target == -1 || dictVal == -1)
							break;
						if(target == dictVal){
							transferBuffer[transferCount] = possibleMatches[p];
							transferCount++;
						}
						
					}
					if(transferCount > 0){
						possibleSize++;
						delete[] possibleMatches;
						possibleMatches = NULL;
						possibleMatchesSize = transferCount;
						possibleMatches = new int[possibleMatchesSize];
						for(int i=0; i<transferCount; i++){
							possibleMatches[i] = transferBuffer[i];
						}
					}
					delete[] transferBuffer;
					transferBuffer = NULL;
					if(transferCount <= 0) break;
				}
			}
			
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Processed %ld bytes\n\033[0m", possibleSize);
			#endif
			if(possibleSize <= 0){
				if(this->lookahead[0] == -1) return false;
				this->compressedCount = 1;
				this->insertLiteral(1);
				this->movDictionary(lookahead[0]);
			}else if(possibleSize <= 3){
				for(int i=0; i<possibleSize && i<this->lookahead_s; i++){
					this->insertLiteral(1);
					this->movDictionary(lookahead[i]);
				}
				this->compressedCount = possibleSize;
			}else{
				this->insertToken(possibleSize, possibleMatches[0]);
				for(int i=0; i<possibleSize; i++){
					if(this->lookahead[i] == -1) break;
					this->movDictionary(this->lookahead[i]);
				}
				this->compressedCount = possibleSize;
			}
			
			return true;
		}
	public:
		char *out;
		size_t out_s;

		LZSSCompression(){
			this->init();
			this->setDictionarySize(256);
			this->setLookaheadSize(256);
			
		}

		~LZSSCompression(){
			this->destroyOut();
			this->destroyLookahead();
			this->destroyDictionary();
		}

		// this may be depricated.
		void setMatchMinimum(size_t min){
			this->matchMinimum = min;
		}

		void setDictionarySize(size_t size){
			this->destroyDictionary();
			this->dictionary_s = size;
			this->dictionary = new signed int[size];
			for(int i=0; i<size; i++)
				this->dictionary[i] = -1;
		}
		size_t getDictionarySize(void){
			return this->dictionary_s;
		}

		void setLookaheadSize(size_t size){
			this->destroyLookahead();
			this->lookahead_s = size;
			this->lookahead = new signed int[size];
			for(int i=0; i<size; i++)
				this->lookahead[i] = -1;
		}
		size_t getLookaheadSize(void){
			return this->lookahead_s;
		}

		int getErrorCode(void){
			return this->error;
		}

		std::string getErrorMsg(void){
			switch(this->error){
				case -1: return "NO ERROR";
				case 0x200: return "insertLiteral(size_t count) - this->lookahead is null.";
				case 0x201: return "insertLiteral(size_t count) - this->lookahead <= 0, treating it as null.";
				case 0x202: return "insertToken(int size, int distance) - this->out is null. It shouldn't be.";
				case 0x203: return "insertToken(int size, int distance) - this->out_s <= 0, it shouldn't be, treating as null.";
				case 0x204: return "insertToken(int size, int distance) - size is > 0xff. We don't support this yet.";
				case 0x205: return "insertToken(int size, int distance) - distance is > 0xff. We don't support this yet.";
				case 0x100: return "compress(char *data, size_t dataSize) - data is null.";
				case 0x101: return "compress(char *data, size_t dataSize) - dataSize is <= 0. data treated as null.";
				case 0x110: return "compress(char *data, size_t dataSize) - this->lookahead is null.";
				case 0x111: return "compress(char *data, size_t dataSize) - this->lookahead_s is <= 0. lookahead treated as null.";
				case 0x120: return "compress(char *data, size_t dataSize) - this->dictionary is null.";
				case 0x121: return "compress(char *data, size_t dataSize) - this->dictionary_s is <= 0. dictionary treated as null.";
				case 0x300: return "decompress(char *data, size_t dataSize) - data is null.";
				case 0x301: return "bool decompress(char *data, size_t dataSize) - dataSize <= 0, treating data as null.";
				default: return "unknown error.";
			}
		}
	
		bool compress(char *data, size_t dataSize){
			if(data == NULL){
				this->error = 0x100;
				return false;
			}
			if(dataSize <= 0){
				this->error = 0x101;
				return false;
			}
			if(this->lookahead == NULL || this->lookahead_s <= 0){
                                this->error = this->lookahead == NULL ? 0x110 : 0x111;
                                return false;
                        }
                        if(this->dictionary == NULL){
                                this->error = this->dictionary == NULL ? 0x120 : 0x121;
                                return false;
                        }
			this->overflowOffset = 0;
			
			// init lookahead
			for(int i=0; i<this->lookahead_s || i<dataSize; i++)
                                this->shiftLookahead(data, dataSize, i);
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("\033[35;1m[DBG] Starting lookahead buffer: ");
				this->printLookahead();
				printf("\033[0;m");
			#endif
			// process data
			for(int i=0; i<dataSize; i++){
				this->encode();
                                for(int j=0; j<this->compressedCount; j++)
                                        this->shiftLookahead(data, dataSize, i+this->lookahead_s+j);
				#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("\033[35;1m[DBG] ");
				this->printLookahead();
				printf("\033[0m");
				#endif
                                i = i + this->compressedCount-1;

			}
			return true;
		}

		bool decompress(char *data, size_t dataSize){
			if(data == NULL){
				this->error = 0x300;
				return false;
			}
			if(dataSize <= 0){
				this->error = 0x301;
				return false;
			}
			std::string obuf = "";
			size_t obuf_s=0;
			for(int i=0; i<dataSize; i++){
				
			}
			
			return true;
		}
};
