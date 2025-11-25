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
		
		void destroyOut(void){
#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
	printf("\033[35;1m[DBG] Destroying output buffer.\n\033[0m");
#endif
			if(this->out != NULL){
				delete[] this->out;
				this->out = NULL;
			}
			this->out_s = 0;
		}
		void destroyDictionary(void){
#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
	printf("\033[35;1m[DBG] Destroying dictionary buffer.\n\033[0m");
#endif
			if(this->dictionary != NULL){
				delete[] this->dictionary;
				this->dictionary = NULL;
			}
			this->dictionary_s = 0;
		}
		void destroyLookahead(void){
#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
	printf("\033[35;1m[DBG] Destroying lookahead buffer.\n\033[0m");
#endif
			if(this->lookahead != NULL){
				delete[] this->lookahead;
				this->lookahead = NULL;
			}
			this->lookahead_s = 0;
		}

		void init(void){
#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
	printf("\033[35;1m[DBG] initalizing LZSS variables.\n\033[0m");
#endif
			this->error = -1;
			this->out = NULL;
			this->out_s = 0;
			this->dictionary = NULL;
			this->dictionary_s = 0;
			this->lookahead = NULL;
			this->lookahead_s = 0;
			this->matchMinimum = 5;
			this->compressedCount = 0;
		}


		bool movDictionary(signed int val){
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Moving %c into the dictionary buffer...\n\033[0m", (char)val);
			#endif
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
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Appending EMPTY to lookahead buffer\n\033[0m");
			#endif
				this->lookahead[this->lookahead_s-1] = -1;
			}else{
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] appending %c into lookahead buffer\n\033[0m", data[index]);
			#endif
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
	public:
		char *out;
		size_t out_s;

		LZSSCompression(){
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Constructing LZSSCompression class.\n\033[0m");
			#endif
			this->init();
			this->setDictionarySize(64);
			this->setLookaheadSize(64);
			
		}

		~LZSSCompression(){
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Deconstructing LZSSCompression class.\n\033[0m");
			#endif
			this->destroyOut();
			this->destroyLookahead();
			this->destroyDictionary();
		}

		void setMatchMinimum(size_t min){
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Setting Match Minimum to %ld\n\033[0m", min);
			#endif
			this->matchMinimum = min;
		}

		void setDictionarySize(size_t size){
			this->destroyDictionary();
			this->dictionary_s = size;
			this->dictionary = new signed int[size];
			for(int i=0; i<size; i++)
				this->dictionary[i] = -1;
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Setting dictionary buffer size to %ld\n\033[0m", size);
			#endif
		}

		void setLookaheadSize(size_t size){
			this->destroyLookahead();
			this->lookahead_s = size;
			this->lookahead = new signed int[size];
			for(int i=0; i<size; i++)
				this->lookahead[i] = -1;
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Set lookahead size to %ld.\n\033[0m", size);
			#endif
		}

		int getErrorCode(void){
			return this->error;
		}

		std::string getErrorMsg(void){
			switch(this->error){
				case -1: return "NO ERROR";
				case 0x100: return "compress(char *data, size_t dataSize) - data is null.";
				case 0x101: return "compress(char *data, size_t dataSize) - dataSize is <= 0. data treated as null.";
				case 0x110: return "compress(char *data, size_t dataSize) - this->lookahead is null.";
				case 0x111: return "compress(char *data, size_t dataSize) - this->lookahead_s is <= 0. lookahead treated as null.";
				case 0x112: return "compress(char *data, size_t dataSize) - this->lookahead_s is > dataSize. Suggested size: 11.";
				case 0x120: return "compress(char *data, size_t dataSize) - this->dictionary is null.";
				case 0x121: return "compress(char *data, size_t dataSize) - this->dictionary_s is <= 0. dictionary treated as null.";
				case 0x200: return "compress(char *data, size_t dataSize) - dFoundIndex < -1. There appears to have been an underflow.";
				case 0x300: return "decompress(char *data, size_t dataSize) - data is null.";
				case 0x301: return "bool decompress(char *data, size_t dataSize) - dataSize <= 0, treating data as null.";
				default: return "unknown error.";
			}
		}
	
		bool compress(char *data, size_t dataSize){
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] Compressing %ld bytes.\n\033[0m", dataSize);
			#endif
			if(data == NULL || dataSize <= 0){
				this->error = data == NULL ? 0x100 : 0x101;
				return false;
			}
			if(this->lookahead == NULL || this->lookahead_s <= 0){
				this->error = this->lookahead == NULL ? 0x110 : 0x111;
				return false;
			}else if(this->lookahead_s > dataSize){
				this->error = 0x112;
				return false;
			}
			if(this->dictionary == NULL){
				this->error = this->dictionary == NULL ? 0x120 : 0x121;
				return false;
			}
			
			this->destroyOut();

			std::string obuf="";
			size_t obuf_s = 0;
			for(int i=0; i<this->lookahead_s && i<dataSize; i++){
				this->shiftLookahead(data, dataSize, i);
			}
			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] ");
			this->printLookahead();
			printf("\033[0m");
			#endif
			
			for(int i=0; i<dataSize; i++){
				std::string match = this->dictToLookCheck();
				#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("\033[35;1m[DBG] Match : '%s'\n\033[0m", match.c_str());
				#endif
				obuf += match;
				obuf_s += match.length();
				for(int j=0; j<this->compressedCount; j++)
					this->shiftLookahead(data, dataSize, i+this->lookahead_s+j);
				i = i + this->compressedCount-1;
				#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
				printf("\033[35;1m[DBG] ");
				this->printLookahead();
				printf("[DBG] ");
				this->printDictionary();
				printf("\033[0m");
				#endif
			}	

			#if LZSSCOMPRESSION_VERBOSE_DEBUG == 1
			printf("\033[35;1m[DBG] ");
			for(int i=0; i<obuf_s; i++){
				printf("%c", obuf[i]);
			}printf("\n\033[0m");
			#endif

			this->destroyOut();
			this->out_s = obuf_s;
			this->out = new char[obuf_s];
			for(int i=0; i<obuf_s; i++){
				out[i] = obuf[i];
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
