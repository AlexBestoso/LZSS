class LZSSCompression{
	private:
		signed int *dictionary;
		size_t dictionary_s;
		//signed int dictionaryAvailable;
		
		signed int *lookahead;
		size_t lookahead_s;
		signed int lookaheadAvailable;

		signed int error;
		int compressedCount;

		int overflowOffset;
		
		void destroyOut(void){
			if(this->out != NULL){
				delete[] this->out;
				this->out = NULL;
			}
			this->out_s = 0;
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
			this->compressedCount = 0;
			this->overflowOffset = 0;
			this->lookaheadAvailable=0;
			//this->dictionaryAvailable=0;
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
				this->lookaheadAvailable--;
			}else{
				this->lookahead[this->lookahead_s-1] = (signed int)data[index];
				this->lookaheadAvailable++;
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
		
		bool resizeOutput(size_t newSize){
			if(this->out == NULL) return false;
			if(this->out_s <= 0) return false;
			
			size_t oldSize = this->out_s;
			char *oldBuff = new char[oldSize];
			for(int i=0; i<this->out_s; i++)
				oldBuff[i] = this->out[i];

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
		bool insertLiteral(size_t lookIndex){
			if(this->lookahead == NULL){
				this->error = 0x200;
				return false;
			}
			if(this->lookahead_s <= 0){ 
				this->error = 0x201;
				return false;
			}

			// modify this section so that if the offset if not a perfect byte, no new char is added.
			int start = this->out_s <= 0 ? 0 : this->out_s-1;
			if(this->out == NULL || this->out_s <= 0){
				this->destroyOut();
				if((this->overflowOffset%8) == 0)
					this->out_s += 1+1;
				else
					this->out_s += 1;

				this->out = new char[this->out_s];
			}else{
				size_t newOutSize = this->out_s;
				if((this->overflowOffset%8) == 0){
					newOutSize += 1+1;
					start++;
				}else{
					newOutSize += 1;
				}
				this->resizeOutput(newOutSize);
			}
			if(start>=this->out_s || start<0){
				return false;
			}
			if(lookIndex >= this->lookahead_s || lookIndex < 0){
				return false;
			}
			if(this->lookaheadAvailable <= 0){
				this->lookaheadAvailable = 0;
				return true;
			}
			signed int lookchar = this->lookahead[lookIndex];
	
			// we need to account for when offset + 1 is equal to 8, which results in data loss...
			if(1+this->overflowOffset < 8){
				//         flag                         remove excess       shift to fit container
				out[start] += (0x00<<(7-this->overflowOffset)) + ((lookchar&0xff) >> (1+this->overflowOffset));
			}else{
				out[start] += (0x00<<(7-this->overflowOffset));
			}

			//  add remainder to next output char.
			start++;
			if(start<this->out_s){
				if(1+this->overflowOffset < 8){
					//            clear used bits
					int clearer = 0xff >> (7-this->overflowOffset);
					lookchar &= clearer;
					//            add unused bits
					out[start] += lookchar << (7-this->overflowOffset);
				}else{
					out[start] = lookchar;
				}
			}
			start--;			
			// ajust for the shifting offset position
			this->overflowOffset = (this->overflowOffset + 1) % 8;

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

			int start = this->out_s <= 0 ? 0 : this->out_s-1;
			size_t newOutSize = this->out_s;
			if((this->overflowOffset%8) == 0){
				newOutSize += 1+1;
				start++;
			}else{
				newOutSize += 1;
			}
			this->resizeOutput(newOutSize);

			// inject the size.
			if(1+this->overflowOffset < 8){
				out[start]  += (0x01<<(7-this->overflowOffset)) + ((size&0xff) >> (1+this->overflowOffset));
			}else{
				out[start]  += (0x01<<(7-this->overflowOffset));
			}
			start++;
			if(start >= this->out_s) return false;
			int clearer=0xff;
			if(1+this->overflowOffset < 8){
				clearer = 0xff >> (7-this->overflowOffset);
                        	size &= clearer;
                        	out[start] = size << (7-this->overflowOffset);
			}else{
				out[start] = size;
			}
			
			this->resizeOutput(this->out_s+1);
			// inject the distance (doesn't change overflow, full byte.
		
			if(1+this->overflowOffset < 8){	
				out[start]  += ((distance&0xff) >> (1+this->overflowOffset));
				start++;
				if(start >= this->out_s) return false;
				clearer = 0xff >> (7-this->overflowOffset);
                        	distance &= clearer;
                        	out[start] = distance << (7-this->overflowOffset);
			}else{
				start++;
				out[start] = distance;
			}
			this->overflowOffset = (this->overflowOffset + 1) % 8;

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
				if(this->lookaheadAvailable <= 0){
					this->compressedCount = 1;
					return true;
				}
				signed int target = this->lookahead[l];
				if(possibleSize == 0 && l > 0) break; // no matches found

				if(possibleSize == 0){
					for(int d=0; d<this->dictionary_s; d++){
						//if(this->dictionaryAvailable <= 0) break; // This may never actually happen.
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
			if(possibleSize <= 0){
				this->compressedCount = 1;
				this->insertLiteral(0);
				this->movDictionary(lookahead[0]);
			}else if(possibleSize <= 3){
				for(int i=0; i<possibleSize && i<this->lookahead_s; i++){
					this->insertLiteral(i);
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

		void setDictionarySize(size_t size){
			this->destroyDictionary();
			this->dictionary_s = size;
			if(size <= 0) return;
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
			if(size <= 0) return;
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
                        if(this->dictionary == NULL || this->dictionary_s <= 0){
                                this->error = this->dictionary == NULL ? 0x120 : 0x121;
                                return false;
                        }
			this->overflowOffset = 0;
			this->lookaheadAvailable = 0;
			// init lookahead
			for(int i=0; i<this->lookahead_s && i<dataSize; i++){
                                //this->shiftLookahead(data, dataSize, i);
				this->lookahead[i] = (signed int)data[i];
				this->lookaheadAvailable++;
			}
			// process data
			for(int i=0; i<dataSize; i++){
				bool encCode = this->encode();
				if(!encCode && this->error > -1){
					printf("Error : %s\n", this->getErrorMsg().c_str());
					return false;
				}
                                for(int j=0; j<this->compressedCount; j++)
                                        this->shiftLookahead(data, dataSize, i+this->lookahead_s+j);
                                i = i + this->compressedCount-1;
				if(!encCode && this->error == -1){
					printf("Failed Encoding break.\n");
					break;
				}

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
			this->overflowOffset = 0;
			this->destroyOut();
			
			for(int i=0; i<dataSize; i++){
				if(i == dataSize-1 && this->overflowOffset != 0){
					break;
				}else if(i > 0 && (7-this->overflowOffset) == 7){
					i++;
					if(i>=dataSize){
						return true;
					}
				}
				int flag = (data[i] & 0xff) >> (7-this->overflowOffset) & 0x01;
				if(flag == 0){ // literal
					char value = ((data[i]&0xff) << (1+this->overflowOffset)) & 0xff;
					i++;
					if(i < dataSize){
						value += ((data[i]&0xff) >> (7-this->overflowOffset)) & 0xff;
					}
					i--;
					this->overflowOffset = (this->overflowOffset+1) % 8;
					if(this->out == NULL || this->out_s <= 0){
						this->out_s = 1;
						this->out = new char[this->out_s];
						this->out[0] = value;
					}else{
						this->resizeOutput(this->out_s+1);
						this->out[this->out_s-1] = value;
					}
				}else{ // token
					char size = ((data[i]&0xff) << (1+this->overflowOffset)) & 0xff;
					i++;
                                        if(i < dataSize){
                                                size += ((data[i]&0xff) >> (7-this->overflowOffset)) & 0xff;
                                        }
                                        
					char distance = ((data[i]&0xff) << (1+this->overflowOffset)) & 0xff;
					distance++; // we have an off by one condition.
					i++;
                                        if(i < dataSize){
                                                distance += ((data[i]&0xff) >> (7-this->overflowOffset)) & 0xff;
                                        }
					int start = this->out_s;
					this->resizeOutput(this->out_s+size);
					for(int j=start; j<this->out_s; j++){
						if(j-distance >= this->out_s || j-distance < 0) break;
						out[j] = out[j-distance];
					}

					this->overflowOffset = (this->overflowOffset+1) % 8;
					i--;
				}
			}
			
			return true;
		}
};
