function TGuyModuleInit(Module) {
    return new Promise((resolve) => {
        TGuyModule(Module).then(
            (Module) => {
                const cwrap = Module['cwrap'];

                class TGuy {
                    static tguy_from_utf8_ex = cwrap('tguy_from_utf8_ex',
                        'number', ['string', 'number', 'number', 'string', 'number',
                            'string', 'number', 'string', 'number', 'string', 'number']);
                    static tguy_from_cstr_arr_ex = cwrap('tguy_from_cstr_arr_ex',
                        'number', ['number', 'number', 'number', 'string', 'number',
                            'string', 'number', 'string', 'number', 'string', 'number']);
                    static tguy_set_frame = cwrap('tguy_set_frame', null, ['number', 'number']);
                    static tguy_get_string = cwrap('tguy_get_string', 'string', ['number', 'number']);
                    static tguy_free = cwrap('tguy_free', null, ['number']);
                    static tguy_get_frames_count = cwrap('tguy_get_frames_count', 'number', ['number']);

                    /**
                     * @param {(string|string[])} text
                     * @param {number} spacing
                     * @param {?string} space
                     * @param {?string} can
                     * @param {?string} right
                     * @param {?string} left
                     */
                    constructor(text, spacing, space, can, right, left) {
                        this.text = text;
                        let tguy_constructor;
                        let len;
                        if (Array.isArray(text)) {
                            if (!text.every(s => typeof (s) === "string")) {
                                throw TypeError("text must only contain strings");
                            }
                            const str_off = Module.HEAPU32.BYTES_PER_ELEMENT * text.length;
                            const size = str_off + text.reduce((acc, val) => acc + Module.lengthBytesUTF8(val) + 1, 0);
                            let arr_mem = Module._malloc(size);

                            let arr = Module.HEAPU32.subarray((arr_mem >> 2), (arr_mem + str_off) >> 2);
                            let str_mem = arr_mem + str_off;
                            text.forEach((val, i) => {
                                arr[i] = str_mem;
                                /* skip written string + terminator */
                                str_mem += 1 + Module.stringToUTF8(val, str_mem, Module.lengthBytesUTF8(val) + 1);
                            });

                            tguy_constructor = TGuy.tguy_from_cstr_arr_ex;
                            len = text.length;
                            text = arr_mem;
                        } else if (typeof (text) == 'string') {
                            tguy_constructor = TGuy.tguy_from_utf8_ex;
                            len = Module.lengthBytesUTF8(text);
                        } else {
                            throw TypeError("text must be a string or array of strings");
                        }

                        this.tgobj = tguy_constructor(
                            text, len, spacing,
                            space, -1,
                            can, -1,
                            right, -1,
                            left, -1
                        );
                        if (typeof (text) === "number") {
                            Module._free(text);
                        }

                        if (this.tgobj === null) {
                            throw RangeError("Out of memory");
                        }

                        this.nframes = TGuy.tguy_get_frames_count(this.tgobj);
                        this.frame = 0;
                    }

                    destructor() {
                        TGuy.tguy_free(this.tgobj);
                    }

                    set_frame(i) {
                        if (i < 0 || i >= this.nframes) {
                            throw RangeError(`Bad index value ${i}, must be in range [0, ${this.nframes})`);
                        }
                        TGuy.tguy_set_frame(this.tgobj, i);
                        this.frame = i;
                    }

                    get_current_frame() {
                        return this.frame;
                    }

                    get_frames_count() {
                        return this.nframes;
                    }

                    toString() {
                        return TGuy.tguy_get_string(this.tgobj, null);
                    }

                    [Symbol.iterator]() {
                        let index = 0;
                        return {
                            next: () => {
                                let value = null;
                                let done = true;

                                if (index < this.nframes) {
                                    this.set_frame(index++);
                                    value = this.toString();
                                    done = false;
                                }
                                return {
                                    value: value,
                                    done: done
                                };
                            }
                        };
                    }
                }

                if (typeof window != "undefined") {
                    window['TGuy'] = TGuy;
                }
                Module['TGuy'] = TGuy;

                resolve(Module);
            });
    });
}
