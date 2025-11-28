class LZSSCompression{
/* Private */
	#dictionary;
	#dictionary_s;
	//signed int dictionaryAvailable;
		
	#lookahead;
	#lookahead_s;
	#lookaheadAvailable;

	#error;
	#compressedCount;

	#overflowOffset;
		
	#destroyOut(){
		this.out = null;
		this.out_s = 0;
	}
	#destroyDictionary(){
		this.#dictionary = null;
		this.#dictionary_s = 0;
	}
	#destroyLookahead(){
		this.#lookahead = null;
		this.#lookahead_s = 0;
	}

	#init(){
		this.#error = -1;
		this.out = null;
		this.out_s = 0;
		this.#dictionary = null;
		this.#dictionary_s = 0;
		this.#lookahead = null;
		this.#lookahead_s = 0;
		this.#compressedCount = 0;
		this.#overflowOffset = 0;
		this.#lookaheadAvailable=0;
		//this.#dictionaryAvailable=0;
	}


	#movDictionary(/*signed int*/ val){
		var transfer = new Array(this.#dictionary_s-1);
		for(var i=0; i<this.#dictionary_s; i++)
			transfer[i] = this.#dictionary[i];
		for(var i=1; i<this.#dictionary_s; i++){
			this.#dictionary[i] = transfer[i-1];
		}
		this.#dictionary[0] = val;
		transfer = null;
		return true;
	}

	#printDictionary(){
		console.log("dictionary : " + this.#dictionary);
	}
	
	#shiftLookahead(/*char **/data, /*size_t */dataSize, /*int */index){
		if(!Array.isArray(data)) return false;
		for(var i=1; i<this.#lookahead_s; i++){
			this.#lookahead[i-1] = this.#lookahead[i];
		}
		if(index >= dataSize || index < 0){
			this.#lookahead[this.#lookahead_s-1] = -1;
			this.#lookaheadAvailable--;
		}else{
			this.#lookahead[this.#lookahead_s-1] = data[index];
			this.#lookaheadAvailable++;
		}
		return true;
	}

	#printLookahead(){
		console.log("lookahead : "+this.#lookahead);
	}
		
	#resizeOutput(/*size_t */newSize){
		if(!Array.isArray(this.out)) return false;
		if(!Number.isInteger(this.out_s) || this.out_s <= 0) return false;
		
		var oldSize = this.out_s;
		var oldBuff = new Array(oldSize);
		for(var i=0; i<this.out_s; i++)
			oldBuff[i] = this.out[i];

		this.#destroyOut();
		this.out_s = newSize;
		this.out = new Array(this.out_s);
		for(var i=0; i<oldSize; i++){
			this.out[i] = oldBuff[i];
		}
		for(var i=oldSize; i<newSize; i++)
			this.out[i] = 0x00;
		oldBuff = null;
		return true;
	}

	/*
	 * A binary 0 before a series of 8 bits means
	 * that the byte should be interpreted as a literal byte.
	 * this constructed bit stream will be injected into this.out.
	 * */
	#insertLiteral(/*size_t */lookIndex){
		if(!Number.isInteger(lookIndex) || lookIndex < 0) throw "insertLiteral(lookIndex) - lookIndex isn't a proper integer.";
		if(!Array.isArray(this.#lookahead)){
			this.#error = 0x200;
			return false;
		}
		if(!Number.isInteger(this.#lookahead_s) || this.#lookahead_s <= 0){ 
			this.#error = 0x201;
			return false;
		}

		// modify this section so that if the offset if not a perfect byte, no new char is added.
		var start = this.out_s <= 0 ? 0 : this.out_s-1;
		if(!Array.isArray(this.out) || !Number.isInteger(this.out_s) || this.out_s <= 0){
			this.#destroyOut();
			if((this.#overflowOffset%8) == 0)
				this.out_s += 1+1;
			else
				this.out_s += 1;

			this.out = new Array(this.out_s);
		}else{
			var newOutSize = this.out_s;
			if((this.#overflowOffset%8) == 0){
				newOutSize += 1+1;
				start++;
			}else{
				newOutSize += 1;
			}
			this.#resizeOutput(newOutSize);
		}
		if(start>=this.out_s || start<0){
			return false;
		}
		if(lookIndex >= this.#lookahead_s || lookIndex < 0){
			return false;
		}
		if(this.#lookaheadAvailable <= 0){
			this.#lookaheadAvailable = 0;
			return true;
		}
		var lookchar = this.#lookahead[lookIndex];
	
		if(!Number.isInteger(this.out[start]))
			this.out[start] = 0;
		// we need to account for when offset + 1 is equal to 8, which results in data loss...
		if(1+this.#overflowOffset < 8){
			//         flag                         remove excess       shift to fit container
			this.out[start] += (0x00<<(7-this.#overflowOffset)) + ((lookchar&0xff) >> (1+this.#overflowOffset));
		}else{
			this.out[start] += (0x00<<(7-this.#overflowOffset));
		}

		//  add remainder to next output char.
		start++;
		if(start<this.out_s){
			if(1+this.#overflowOffset < 8){
				//            clear used bits
				var clearer = 0xff >> (7-this.#overflowOffset);
				lookchar = lookchar & clearer;
				//            add unused bits
				this.out[start] += lookchar << (7-this.#overflowOffset);
			}else{
				this.out[start] = lookchar;
			}
		}
		start--;			
		// ajust for the shifting offset position
		this.#overflowOffset = (this.#overflowOffset + 1) % 8;
		return true;
	}

	/*
	 * A binary 1 before a seires of 8 bits means that we're defining
	 * a token size, and that the next 8 bits are an offset/distance.
	 * this bit stream will be injected into this.out.
	 * */
	// TODO: make this size relative to dictionary size, maybe
	#insertToken(/*int */size, /*int */distance){
		if(!Number.isInteger(size) || size < 0) throw "insertToken(size, distance) - size is an invalid integer value.";
		if(!Number.isInteger(distance) || distance < 0) throw "insertToken(size, distance) - distance is an invalid integer value.";
		if(!Array.isArray(this.out)){ 
			this.#error = 0x202;
			return false;
		}
		if(this.out_s <= 0){
			this.#error = 0x203;
			return false; // this shouldn't be running before insertLiteral
		}
		// we only support a max compression of 255...for now.
		if(size > 0xff){
			this.#error = 0x204;
			return false;
		}
		if(distance > 0xff){
			this.#error = 0x205;
			return false;
		}

		var start = this.out_s <= 0 ? 0 : this.out_s-1;
		var newOutSize = this.out_s;
		if((this.#overflowOffset%8) == 0){
			newOutSize += 1+1;
			start++;
		}else{
			newOutSize += 1;
		}
		this.#resizeOutput(newOutSize);

		// inject the size.
		if(1+this.#overflowOffset < 8){
			this.out[start]  += (0x01<<(7-this.#overflowOffset)) + ((size&0xff) >> (1+this.#overflowOffset));
		}else{
			this.out[start]  += (0x01<<(7-this.#overflowOffset));
		}
		start++;
		if(start >= this.out_s) return false;
		var clearer=0xff;
		if(1+this.#overflowOffset < 8){
			clearer = 0xff >> (7-this.#overflowOffset);
                       	size &= clearer;
                       	this.out[start] = size << (7-this.#overflowOffset);
		}else{
			this.out[start] = size;
		}
			
		this.#resizeOutput(this.out_s+1);
		// inject the distance (doesn't change overflow, full byte.
		
		if(1+this.#overflowOffset < 8){	
			this.out[start]  += ((distance&0xff) >> (1+this.#overflowOffset));
			start++;
			if(start >= this.out_s) return false;
			clearer = 0xff >> (7-this.#overflowOffset);
                       	distance &= clearer;
                       	this.out[start] = distance << (7-this.#overflowOffset);
		}else{
			start++;
			this.out[start] = distance;
		}
		this.#overflowOffset = (this.#overflowOffset + 1) % 8;

		return true;	
	}

	#encode(){
		this.#compressedCount = 0;
		if(!Array.isArray(this.#dictionary)){
			return false;
		}
		if(!Array.isArray(this.#lookahead)){
			return false;
		}

		var possibleMatchesSize = 0;
		var possibleSize = 0;
		var possibleMatches = null;
		var transferBuffer = null;
			
		for(var l=0; l<this.#lookahead_s; l++){
			if(this.#lookaheadAvailable <= 0){
				this.#compressedCount = 1;
				return true;
			}
			var target = this.#lookahead[l];
			if(possibleSize == 0 && l > 0) break; // no matches found

			if(possibleSize == 0){
				for(var d=0; d<this.#dictionary_s; d++){
					//if(this.#dictionaryAvailable <= 0) break; // This may never actually happen.
					if(target == this.#dictionary[d]){
						possibleMatchesSize++;
						if(possibleMatches == null){
							possibleMatches = new Array(possibleMatchesSize);
							possibleMatches[possibleMatchesSize-1] = d;
						}else{
							transferBuffer = new Array(possibleMatchesSize-1);
							for(var i=0; i<possibleMatchesSize-1; i++)
								transferBuffer[i] = possibleMatches[i];
							possibleMatches = null;
							possibleMatches = new Array(possibleMatchesSize);
							for(var i=0; i<possibleMatchesSize-1; i++)
								possibleMatches[i] = transferBuffer[i];
							possibleMatches[possibleMatchesSize-1] = d;
							transferBuffer = null;
						}
							
					}
				}
				if(possibleMatchesSize > 0)
					possibleSize++;
			}else{
				transferBuffer = new Array(possibleMatchesSize);
				var transferCount=0;
				for(var p=0; p<possibleMatchesSize; p++){
					var dIndex = possibleMatches[p]-possibleSize;
					if(dIndex < 0){
						continue; // only a size of 1 is possible here.
					}
					var dictVal = this.#dictionary[dIndex];
					if(target == -1 || dictVal == -1)
						break;
					if(target == dictVal){
						transferBuffer[transferCount] = possibleMatches[p];
						transferCount++;
					}
						
				}
				if(transferCount > 0){
					possibleSize++;
					possibleMatches = null;
					possibleMatchesSize = transferCount;
					possibleMatches = new Array(possibleMatchesSize);
					for(var i=0; i<transferCount; i++){
						possibleMatches[i] = transferBuffer[i];
					}
				}
				transferBuffer = null;
				if(transferCount <= 0) break;
			}
		}
		if(possibleSize <= 0){
			this.#compressedCount = 1;
			this.#insertLiteral(0);
			this.#movDictionary(this.#lookahead[0]);
		}else if(possibleSize <= 3){
			for(var i=0; i<possibleSize && i<this.#lookahead_s; i++){
				this.#insertLiteral(i);
				this.#movDictionary(this.#lookahead[i]);
			}
			this.#compressedCount = possibleSize;
		}else{
			this.#insertToken(possibleSize, possibleMatches[0]);
			for(var i=0; i<possibleSize; i++){
				if(this.#lookahead[i] == -1) break;
				this.#movDictionary(this.#lookahead[i]);
			}
			this.#compressedCount = possibleSize;
		}
		
		return true;
	}

/* Public */
	out;
	out_s;

	constructor(){
		this.#init();
		this.setDictionarySize(256);
		this.setLookaheadSize(256);
			
	}

	destroy(){
		this.#destroyOut();
		this.#destroyLookahead();
		this.#destroyDictionary();
	}

	setDictionarySize(/*size_t */size){
		if(!Number.isInteger(size)) throw "setDictionarySize(size) size isn't an integer.";
		this.#destroyDictionary();
		this.#dictionary_s = size;
		if(size <= 0) return;
		this.#dictionary = new Array(size);
		for(var i=0; i<size; i++)
			this.#dictionary[i] = -1;
	}
	getDictionarySize(){
		return this.#dictionary_s;
	}

	setLookaheadSize(/*size_t */size){
		if(!Number.isInteger(size)) throw "setLookaheadSize(size) size isn't an integer.";
		this.#destroyLookahead();
		this.#lookahead_s = size;
		if(size <= 0) return;
		this.#lookahead = new Array(size);
		for(var i=0; i<size; i++)
			this.#lookahead[i] = -1;
	}
	getLookaheadSize(){
		return this.#lookahead_s;
	}

	getErrorCode(){
		return this.#error;
	}

	getErrorMsg(){
		switch(this.#error){
			case -1: return "NO ERROR";
			case 0x200: return "insertLiteral(size_t count) - this.#lookahead is null.";
			case 0x201: return "insertLiteral(size_t count) - this.#lookahead <= 0, treating it as null.";
			case 0x202: return "insertToken(int size, int distance) - this.out is null. It shouldn't be.";
			case 0x203: return "insertToken(int size, int distance) - this.out_s <= 0, it shouldn't be, treating as null.";
			case 0x204: return "insertToken(int size, int distance) - size is > 0xff. We don't support this yet.";
			case 0x205: return "insertToken(int size, int distance) - distance is > 0xff. We don't support this yet.";
			case 0x100: return "compress(char *data, size_t dataSize) - data is not an array.";
			case 0x101: return "compress(char *data, size_t dataSize) - dataSize is <= 0. data treated as null.";
			case 0x110: return "compress(char *data, size_t dataSize) - this.#lookahead is null.";
			case 0x111: return "compress(char *data, size_t dataSize) - this.#lookahead_s is <= 0. lookahead treated as null.";
			case 0x120: return "compress(char *data, size_t dataSize) - this.#dictionary is null.";
			case 0x121: return "compress(char *data, size_t dataSize) - this.#dictionary_s is <= 0. dictionary treated as null.";
			case 0x300: return "decompress(char *data, size_t dataSize) - data is null.";
			case 0x301: return "bool decompress(char *data, size_t dataSize) - dataSize <= 0, treating data as null.";
			default: return "unknown error.";
		}
	}
	
	compress(/*char **/data, /*size_t */dataSize){
		if(!Array.isArray(data)){
			this.#error = 0x100;
			return false;
		}
		if(!Number.isInteger(dataSize) || dataSize <= 0){
			this.#error = 0x101;
			return false;
		}
		if(!Array.isArray(this.#lookahead) || !Number.isInteger(this.#lookahead_s) || this.#lookahead_s <= 0){
			this.#error = !Array.isArray(this.#lookahead) ? 0x110 : 0x111;
			return false;
		}
		if(!Array.isArray(this.#dictionary) || !Number.isInteger(this.#dictionary_s) || this.#dictionary_s <= 0){
			this.#error = !Array.isArray(this.#dictionary) ? 0x120 : 0x121;
			return false;
		}
		this.#overflowOffset = 0;
		this.#lookaheadAvailable = 0;
		// init lookahead
		for(var i=0; i<this.#lookahead_s && i<dataSize; i++){
			this.#lookahead[i] = data[i]
			this.#lookaheadAvailable++;
		}
		// process data
		for(var i=0; i<dataSize; i++){
			var encCode = this.#encode();
			if(!encCode && this.#error > -1){
				console.log("Error : "+ this.getErrorMsg());
				return false;
			}
                        for(var j=0; j<this.#compressedCount; j++)
                                this.#shiftLookahead(data, dataSize, i+this.#lookahead_s+j);
                        i = i + this.#compressedCount-1;
			if(!encCode && this.#error == -1){
				console.log("Failed Encoding break.\n");
				break;
			}

		}
		return true;
	}

	decompress(/*char **/data, /*size_t */dataSize){
		if(!Array.isArray(data)){
			this.#error = 0x300;
			return false;
		}
		if(!Number.isInteger(dataSize) || dataSize <= 0){
			this.#error = 0x301;
			return false;
		}
		this.#overflowOffset = 0;
		this.#destroyOut();
			
		for(var i=0; i<dataSize; i++){
			if(i == dataSize-1 && this.#overflowOffset != 0){
				break;
			}else if(i > 0 && (7-this.#overflowOffset) == 7){
				i++;
				if(i>=dataSize){
					return true;
				}
			}
			var flag = (data[i] & 0xff) >> (7-this.#overflowOffset) & 0x01;
			if(flag == 0){ // literal
				var value = ((data[i]&0xff) << (1+this.#overflowOffset)) & 0xff;
				i++;
				if(i < dataSize){
					value += ((data[i]&0xff) >> (7-this.#overflowOffset)) & 0xff;
				}
				i--;
				this.#overflowOffset = (this.#overflowOffset+1) % 8;
				if(!Array.isArray(this.out) || this.out_s <= 0){
					this.out_s = 1;
					this.out = new Array(this.out_s);
					this.out[0] = value;
				}else{
					this.#resizeOutput(this.out_s+1);
					this.out[this.out_s-1] = value;
				}
			}else{ // token
				var size = ((data[i]&0xff) << (1+this.#overflowOffset)) & 0xff;
				i++;
                                if(i < dataSize){
                                        size += ((data[i]&0xff) >> (7-this.#overflowOffset)) & 0xff;
                                }
                                        
				var distance = ((data[i]&0xff) << (1+this.#overflowOffset)) & 0xff;
				distance++; // we have an off by one condition.
				i++;
                                if(i < dataSize){
                                        distance += ((data[i]&0xff) >> (7-this.#overflowOffset)) & 0xff;
                                }
				var start = this.out_s;
				this.#resizeOutput(this.out_s+size);
				for(var j=start; j<this.out_s; j++){
					if(j-distance >= this.out_s || j-distance < 0) break;
					this.out[j] = this.out[j-distance];
				}

				this.#overflowOffset = (this.#overflowOffset+1) % 8;
				i--;
			}
		}
		return true;
	}
}
