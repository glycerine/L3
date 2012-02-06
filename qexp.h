//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef NEW_QEXP_H_
#define NEW_QEXP_H_

#include <string.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


struct Tag;

typedef unsigned long ulong;

struct qtree;

typedef qtree sexp_t;

// qqchar : sane replacement for char*

// 
// generalization of sane-strings, D-style array, and dual iterator STL patterns. A pointer & pointer (pp).
//

template <typename T>
struct pp {
    T* b; // beginning
    T* e; // ending: one past the legit value, so size is (_end - _beg)
};

typedef pp<double>  ppdouble;
typedef pp<long>    pplong;

struct tdopout_struct;

struct tokenspan {
    long _b;
    long _e;
    tdopout_struct*  _tdo;

    tokenspan() : _b(0), _e(0), _tdo(0) {}
    tokenspan(long b, long e, tdopout_struct*  tdo) 
        : _b(b), _e(e), _tdo(tdo)
    {
        assert(_e >= _b);
    }

    // in qexp.cpp
    long copyto(char* s, ulong sz);
};



//
// qqchar: never owns the memory it points at, unless you call ownit() 
//
struct qqchar {

    char* b; // beginning
    char* e; // ending: one past the legit value, so size is (_end - _beg)

    // by default we do not own the memory, only if _tag is set, and even then _tag actually owns it.
    // Having _tag lets us do stack allocated qqchar if we really need to, and still handle exceptions.
    Tag*  _owner;
    char  _internal[8]; // small atom storage, not requiring a separate malloc.

    // if b == &_internal[0] then we are using the _internal array, and _owner should be 0.


    qqchar() : b(0),e(0), _owner(0) { bzero(_internal, sizeof(_internal)); }

    qqchar(char* begin, char* end) : b(begin), e(end), _owner(0) { bzero(_internal, sizeof(_internal)); }

    // copies are always references unless we next call ownit().
    qqchar(const qqchar& src);
    qqchar(const char* src);
    qqchar&  operator=(const char* src);
    qqchar&  operator=(const qqchar& src);


    ~qqchar();


    // combine clients doing dup() + release() into one call, str_dup() to get a null-terminated,
    // malloc-ed string that clients must free.
    char* str_dup() {

        // we cannot use strdup directly, b/c we cannot assume we have a null-terminated string.
        // Frequently the string will *not* be \0 terminated.
        long z = sz();
        char* r = (char*)malloc(z+1);

        assert(r);
        memcpy(r, b, z);
        r[z]=0; // null terminate our dups, so we can refer to them as strings.

        return r;
    }


    // ownit: "make your own copy of b"-- on heap, or in _intern
    //  with owner actually owning/being the backup plan in case of exception.
    void ownit(Tag* owner);

    // change ownership; calls ownit(new_owner) if need be.
    void transfer_to(Tag* new_owner);

    // narrow ourselves to not include one layer of double quotes, if applicable
    void dquote_narrow() {
        if (b && *b     =='\"') { ++b; }
        if (e && *(e-1) =='\"') { --e; }
    }

    void triple_quote_narrow() {
        if (b && *b     =='\"') { ++b; }
        if (b && *b     =='\"') { ++b; }
        if (b && *b     =='\"') { ++b; }
        if (e && *(e-1) =='\"') { --e; }
        if (e && *(e-1) =='\"') { --e; }
        if (e && *(e-1) =='\"') { --e; }
    }

    void single_quote_narrow() {
        if (b && *b     =='\'') { ++b; }
        if (e && *(e-1) =='\'') { --e; }
    }



    inline long sz() const { return e-b; }
    inline long len() const { return e-b; }
    inline long size() const { return e-b; }

    inline bool empty() const { return e==b; }

    inline void write(int fd) {     ::write(fd, b, e-b); }

    void write_with_indent(int fd, const char* ind) {
        ulong ni = strlen(ind);
        if (ni==0) return write(fd);

        char* p     = b;
        char* pprev = b;

        while (p < e) {
            if (*p == '\n') {
                ++p;
                ::write(fd, pprev, p-pprev);
                if (p < e) {
                    ::write(fd, ind, ni);
                }
                pprev=p;
            } else {
                ++p;
            }
        }
        if (pprev < e) {
            ::write(fd, pprev, e-pprev);
        }
    }

    void clear();


    //
    // copyto: returns number of char written to dest-ination.
    //
    inline long copyto(qqchar& dest) const {
        long dcap = dest.sz();
        if (dcap <=0) return 0;

        long mycap = sz();
        if (mycap <= dcap) {
            memcpy(dest.b, b, mycap);
            return mycap;
        } else  {
            memcpy(dest.b, b, dcap);
            return dcap;
        }
    }

    //
    // we null terminate our copy if there is space...and we return the
    // number of bytes written so the client call easily
    // do so if they wish.
    //
    inline long copyto(char* dest, long dcap) const {
        if (dcap <=0) return 0;

        long mysz = sz();
        if (mysz <= dcap) {
            memcpy(dest, b, mysz);
            if (mysz < dcap) {
                dest[mysz]=0;
            }
            return mysz;
        } else  {
            memcpy(dest, b, dcap);
            return dcap;
        }
    }


    inline int strcmp(const char* s2) const {
        size_t z = sz();
        size_t z2 = strlen(s2);
        if (z2 > z) { z = z2; }
        // compare with the longer strings length.
        return ::strncmp(b, s2, z);
    }

    inline int strncmp(const char* s2, int n) const {
        return ::strncmp(b, s2, n);
    }

};

std::ostream& operator<<(std::ostream& os, const qqchar& qq);


//
// q : important typedef/macro for later extension/modification/upgrades: wrap all strings in this!
//
typedef qqchar q;


//
// fmt: 
//
typedef qqchar fmt;



// allocate enough memory to hold all the
// code text we'll ever encounter, and just
// point to it.
struct PermaText {

    char* _start; // never changes

    char* _fp; // top of the frame pointer : start of the most recently added command
    char* _sp; // current stack pointer    : end of the most recently added command

    static const long _capacity = 10*1024*1024;
    char* _stop;

    PermaText() {
        _start = (char*)calloc(1,_capacity);
        _fp = _sp = _start;

        _stop = _start + _capacity;
    }

    ~PermaText() {
        ::free(_start);
    }

    inline char* start() { return _start; }

    inline char* end()   { return _sp; }
    inline char* frame() { return _fp; }

    void add(const char* addme) {
        _fp = _sp; // advance

        // and copy in new string.
        _sp = stpcpy(_sp, addme);

        // put in a newline, so that triple quoted strings come out right.
        *_sp++ = '\n';

        // make the strings all one text by not doing:
        // ++_sp; 

        if (_sp > _stop) {
            printf("VERY BAD INTERNAL ERROR: PermaText out of memory: try increasing the _capacity, currently %.3f MB\n", ((double)_capacity) / (1024*1024));
            assert(0);
            exit(1);
        }
    }

    void add_newline() {
        const static char nothing[] = "";
        add(&nothing[0]);
    }

    long len() { return _sp - _fp; }

#ifdef _MACOSX

    // in Mac, use fgetln (BSD lineage)
    long add_line_from_fp(FILE* fp) {
        _fp = _sp; // advance

        size_t gotcount = 0;

        //BSD/Mac only, so got to getline instead of gfetln:
        char* line = fgetln(fp, &gotcount);

        if (line && gotcount) {
            memcpy(_fp, line, gotcount);
            _sp += gotcount;
            return gotcount;
        } else {
            return 0;  // error or eof
        }
    }

#else
    // not _MACOSX, linux doesn't have fgetln, so use getline (GNU/posix).
    //  getline needs a ::free() afterwards, so give it it's own method:
    long add_line_from_fp(FILE* fp) {
        _fp = _sp; // advance

        char*   line = 0;
        size_t  pre_size_of_line = 0;
        size_t  gotcount = getline(&line, &pre_size_of_line, fp);

        if (line && gotcount) {
            memcpy(_fp, line, gotcount);
            _sp += gotcount;
            ::free(line);
            return gotcount;
        } else {
            return 0; // error or eof
        }
    }
#endif

};

extern PermaText* permatext;



#endif /* NEW_QEXP_H_ */

