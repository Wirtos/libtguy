(function () {
    const cwrap = Module['cwrap'],
        TYPE_PTR = '*',
        PTR_SIZE = Module['POINTER_SIZE'],
        NUMBER = Module.TGUY_DO_NOT_UNROLL = 'number',
        STRING = Module.TGUY_DO_NOT_UNROLL = 'string',
        ctor_args = [NUMBER, NUMBER, STRING, NUMBER, STRING, NUMBER, STRING, NUMBER, STRING],
        tguy_from_utf8_ex = cwrap('tguy_from_utf8_ex', NUMBER, [STRING, ...ctor_args]),
        tguy_from_cstr_arr_ex = cwrap('tguy_from_cstr_arr_ex', NUMBER, [NUMBER, ...ctor_args]),
        tguy_set_frame = cwrap('tguy_set_frame', null, [NUMBER, NUMBER]),
        tguy_get_string = cwrap('tguy_get_string', STRING, [NUMBER, NUMBER]),
        tguy_free = cwrap('tguy_free', null, [NUMBER]),
        tguy_get_frames_count = cwrap('tguy_get_frames_count', NUMBER, [NUMBER]),
        tguy_get_arr = cwrap('tguy_get_arr', NUMBER, [NUMBER, NUMBER]),
        lengthBytesUTF8 = Module['lengthBytesUTF8'],
        stringToUTF8 = Module['stringToUTF8'],
        UTF8ToString = Module['UTF8ToString'],
        malloc = Module['_malloc'],
        free = Module['_free'],
        setValue = Module['setValue'],
        getValue = Module['getValue'];

    class TGuy {
        /**
         * @param {(string|Array<string>)} text
         * @param {number=} spacing
         * @param {?string=} space
         * @param {?string=} can
         * @param {?string=} right
         * @param {?string=} left
         */
        constructor(text, spacing = 4, space = null, can = null, right = null, left = null) {
            /** @type {(string|Array<string>)} */
            this.text = text;
            /** @type {number|null} */
            this.tgobj = null;
            /** @type {number} */
            this.nframes = 0;
            /** @type {number} */
            this.frame = 0;
            /** @type {function((string|number),number,number,string?,number,string?,number,string?,number,string?,number)}*/
            let tguy_constructor;
            /** @type {number} */
            let len;

            if (text.length === 0) {
                /* in case empty array is passed */
                text = '';
            }

            if (Array.isArray(text)) {
                if (!text.every(s => typeof s === 'string')) {
                    throw TypeError('text must only contain strings');
                }

                /* this allocated block of memory is in form of
                struct {
                    char *arr[text.length];
                    char str_mem[strings_size_bytes];
                } */
                const arr_size_bytes = text.length * PTR_SIZE;
                const strings_size_bytes = text.reduce(((acc, val) => acc + lengthBytesUTF8(val) + 1), 0);
                /* allocate array of char * + memory for storing strings including nul terminators */
                let arr_mem = malloc(arr_size_bytes + strings_size_bytes);
                if (!arr_mem) {
                    throw RangeError('No memory left for array allocation');
                }

                let str_mem = arr_mem + arr_size_bytes;
                text.forEach((chrs, i) => {
                    /* point arr to memory of str_mem, where we write current string */
                    setValue(arr_mem + (i * PTR_SIZE), str_mem, TYPE_PTR);
                    /* advance memory by number of bytes written + nul terminator */
                    str_mem += stringToUTF8(chrs, str_mem, lengthBytesUTF8(chrs) + 1) + 1;
                });

                tguy_constructor = tguy_from_cstr_arr_ex;
                len = text.length;
                text = arr_mem;
            } else if (typeof text === 'string') {
                tguy_constructor = tguy_from_utf8_ex;
                len = lengthBytesUTF8(text);
            } else {
                throw TypeError('text must be a string or array of strings');
            }

            this.tgobj = tguy_constructor(text, len, spacing, space, -1, can, -1, right, -1, left, -1);
            if (typeof text === 'number') {
                free(text);
            }

            if (this.tgobj === null) {
                throw RangeError('Out of memory');
            }

            this.nframes = tguy_get_frames_count(this.tgobj);
            this.frame = 0;
            this.first_element_frames_count = (spacing + 1) * 2;
        }

        /** @returns {void} */
        destructor() {
            tguy_free(this.tgobj);
        }

        /**
         * @param {number} i
         * @returns {void}
         */
        set_frame(i) {
            if (i < 0 || i >= this.nframes) {
                throw RangeError(`Bad index value ${i}, must be in range [0, ${this.nframes})`);
            }
            tguy_set_frame(this.tgobj, i);
            this.frame = i;
        }

        /** @returns {number} */
        get_current_frame() {
            return this.frame;
        }

        /** @returns {number} */
        get_frames_count() {
            return this.nframes;
        }

        /** @return {Array<{str:string,len:number}>} */
        get_arr() {
            let arrPtr = tguy_get_arr(this.tgobj), outArr = [];

            while (1) {
                const strPtr = getValue(arrPtr, TYPE_PTR);
                if (strPtr === 0) break;
                const strLen = getValue(arrPtr + PTR_SIZE, TYPE_PTR);
                outArr.push({'str': UTF8ToString(strPtr, strLen, true), 'len': strLen});
                arrPtr += (PTR_SIZE * 2);
            }

            return outArr;
        }

        /** @returns {number} */
        get_first_frame_for_element(element_index) {
            return element_index * (element_index + this.first_element_frames_count - 1);
        }

        /** @returns {string} */
        toString() {
            return tguy_get_string(this.tgobj, null);
        }

        /** @returns {{next: function(): {value: string?, done: boolean}}} */
        [Symbol.iterator]() {
            let index = 0;
            return {
                next: () => {
                    if (index < this.nframes) {
                        this.set_frame(index++);
                        return {
                            value: this.toString(), done: false
                        };
                    }
                    return {
                        value: null, done: true
                    };
                }
            };
        }
    }

    const prot = TGuy.prototype;
    /* make so that closures don't optimize or rename methods out */
    prot['destructor'] = prot.destructor;
    prot['set_frame'] = prot.set_frame;
    prot['get_current_frame'] = prot.get_current_frame;
    prot['get_frames_count'] = prot.get_frames_count;
    prot['get_arr'] = prot.get_arr;
    prot['get_first_frame_for_element'] = prot.get_first_frame_for_element;

    Module['TGuy'] = TGuy;
})();
