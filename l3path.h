//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3PATH_H
#define L3PATH_H

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "qexp.h"
#include <string.h>
#include <string>
#include <assert.h>


//////////////////////////
//
// l3path: easy path additions with +=, subtraction with pop()
//
//////////////////////////

// in l3obj.cpp : here to avoid cyclic includes
void l3str_reinit_helper(const sexp_t* sexp, char*& p, char** pbuf, int bufsz);


#define L3PATH_MAX (PATH_MAX-1)

template<int str_max>
struct l3str {
  // INVAR: p should always point to '\0'
  // INVAR: *(p-1) should never be '/' (so long as p > buf)

  char buf[str_max];
  char*  p;
    //  int  bufsz;

  // after using buf manually, re-establish a correcct value for p using set_p_from_strlen()
  void set_p_from_strlen() {
    p = &buf[0] + strlen(buf);
  }

  void dump(const char* pre = 0) {
    if (pre) {
      printf("%s : %s\n",pre,buf);
    } else {
      printf("%s\n",buf);
    }
  }

  l3str(const char* s = 0) {
      //    bufsz = STR_MAX;
    init(s);
  }

  l3str(const l3str& src) {
      //    bufsz = src.bufsz;
    memcpy(buf,src.buf,str_max);
    p = &buf[0] + (src.p - &(src.buf[0]) );
  }

  l3str(const qqchar& src) {
      bzero(buf, sizeof(buf));
      p = &buf[0];
      pushq((qqchar&)src);
  }

#if 0
    l3str(const sexp_t* sexp);

    void reinit(const sexp_t* sexp);
#endif

  l3str(const sexp_t* sexp) {
    reinit(sexp);
  }

  void reinit(const sexp_t* sexp) {

      char* pbuf = &buf[0];
      char** ppbuf = &pbuf;
      l3str_reinit_helper(sexp, p, ppbuf, bufsz());
}



  // have to include an ignored int param to
  // differentiate this constructor from the
  // copy constructor.
  // But we'll actually use it if we need
  // a bigger buffer...let that be the buf size then.
  l3str(int bufsize_defaultifzero, const char* fmt, ...) {
      /*
    if (bufsize_defaultifzero == 0) {
      bufsz = PATH_MAX;
    } else {
      printf("error: bufsize of non-zero not yet implemented!\n");
      exit(1);
      bufsz = bufsize_defaultifzero;
    }
      */
    bzero(buf, sizeof(buf));
    p = &buf[0];
    
    va_list ap;
    va_start(ap, fmt);
    
    int n= vsprintf(p, fmt,  ap);
    p +=n;
  }


  l3str& operator=(const l3str& src) {
    if (&src == this) return *this;

    memcpy(buf, src.buf, str_max);
    p = &buf[0] + (src.p - &(src.buf[0]) );

    return *this;
  }

  l3str(const std::string& s) {
    init(s.c_str());
  }

  void init(const char* s = 0) {
    bzero(buf, sizeof(buf));
    p = &buf[0];
    if (s) {
      p += ::sprintf(p,"%s",s);
    }
  }

  void clear() {
    p = &buf[0];
    *p = '\0';
  }

  // delete n letters from the right
  void rdel(long n) {
    assert(n >=0);
    if (n >= (p- &buf[0])) {
      p = &buf[0];
    } else {
      p = p - n;
    }
    *p = '\0';
  }
  
  void reinit(const char* fmt, ...) {
    bzero(buf, sizeof(buf));
    p = &buf[0];

    va_list ap;
    va_start(ap, fmt);

    if (fmt) {    
      int n= vsprintf(p, fmt,  ap);
      //      p +=n;
      p = buf + n;
    }
  }
  
    void reinit(const qqchar& s) {
        clear();
        pushq(s);
    }

  long len()    { return p-(&buf[0]); }
  long length() { return len(); }
  long size()   { return len(); }
  
  int pushf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    int n= vsprintf(p, fmt,  ap);
    p +=n;
    return n;
  }

    long pushq(const qqchar& s) {
        qqchar dest(p, &buf[bufsz()-2]);
        long n = s.copyto(dest);
        p += n;
        *p = 0;
        return n;
    }

    qqchar as_qq() {
        qqchar ret(&buf[0],p);
        return ret;
    }

    // repeated pushf - useful for indentation levels.
    int rep_pushf(int repcount, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        assert(repcount >= 0);
        if (repcount <= 0) return 0; // for release build.

        char* pstart= p;
        for (long i =0 ; i < repcount; ++i ) {
            p += vsprintf(p, fmt,  ap);
        }

        return (p-pstart);
  }


  int prepushf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    l3str tmp;
    int n= vsprintf(tmp.buf, fmt,  ap);
    tmp.p +=n;

    tmp.pushf("%s",buf);
    init(tmp());

    return n;
  }

  bool last_char_slash() {
    if (p > buf && *(p-1) == '/') {
      return true;
    }
    return false;
  }

  void operator+=(void* v) {
    char fmt[] = "/%p";
    if (last_char_slash()) {
      strcpy(fmt,"%p");
    }
    pushf(fmt,v);
  }

  void operator+=(const char* s) {
    char fmt[] = "/%s";
    if (last_char_slash() || s[0]=='/') {
      strcpy(fmt,"%s");
    }
    pushf(fmt,s);
  }

  void operator+=(const std::string& str) {
    push(str);
  }

  void outln() {
    printf("%s\n",buf);
  }
    
    // put an indent in front of each printed \n
    void outln_indent(char* ind) {

        char* start = &buf[0];
        char* last = p-1;

        if (strlen(ind)==0) {
            printf("%s\n",buf);

        } else {

            //find the next \n and print to there with the given indent.
            for (char* cur = start; cur <=last; ++cur ) {

                if (*cur == '\n') {
                    
                    *cur = '\0'; // temp terminator replaces newline.
                    printf("%s%s\n",ind,start);
                    *cur = '\n'; // replace the newline
                    start = cur+1;
                    cur   = cur+1;
                }
            }
            // catch and print the last non newline terminated segment too:
            if (start <=last) {
                printf("%s%s\n",ind,start);
            }
        }
    } //end outln_indent


  void out() {
    printf("%s",buf);
  }
  void errln() {
    fprintf(stderr,"%s\n",buf);
  }
  void err() {
    fprintf(stderr,"%s",buf);
  }

  long grep(const char* needle) {
    long nlen = strlen(needle);
    long mylen = len();
    if (nlen > mylen) return -1; // impossible
    char* k = &buf[0];
    long doneindex = mylen - nlen +1;
    for (long i = 0; i < doneindex; ++i, ++k) {
      if (0==strcmp(&buf[i], needle)) return i;
    }
    return -1;
  }

  void push(const char* s) {
    char fmt[] = "/%s";
    if (last_char_slash() || s[0]=='/') {
      strcpy(fmt,"%s");
    }
    pushf(fmt,s);    
  }

  void push(const std::string& str) {
    push(str.c_str());
  }
  
  char* operator()() {
    return buf;
  }
  
  std::string  tostring() {
    return buf;
  }


  int eq(const char* s) {
      if (0==strncmp(buf,s,str_max)) return 1;
      return 0;
  }

  // find the last file or dir and remove it
  void pop() {
    char* newp = p -1;
    while (newp > &buf[0] && *newp != '/') --newp;

    if (newp > &buf[0]) {
      assert(*newp == '/');
    }

    *newp='\0';
    p = newp;

  }

    // pop off one dotted name
  void pop_dot() {
    char* newp = p -1;
    while (newp > &buf[0] && *newp != '.') --newp;

    if (newp > &buf[0]) {
      assert(*newp == '.');
    }

    *newp='\0';
    p = newp;

  }

    // remove nchar from the end 
  void popchar(long nchar) {
    assert(nchar >= 0);
    if (nchar == 0) return;
    if (nchar >= len()) {
        clear();
        return; 
    }

    char* newp = p - nchar;
    *newp='\0';
    p = newp;

  }


  void chompslash() {
    char* newp = p -1;
    if(newp > &buf[0] && *newp == '/'){
      *newp='\0';
      p = newp;
    }
  }


  void chomp() {
    char* newp = p -1;
    while(newp > &buf[0] && ( (*newp) =='\n' || (*newp) =='\t' || (*newp) ==' ')) {
      (*newp) ='\0';
      newp--;
    }
    p = newp+1;
  }

  void trimr() { chomp(); }
  void triml() {
      long len = p - &buf[0];
      char* fnws = &buf[0];
      while (fnws < p && *fnws) {
          if (*fnws == ' ' || *fnws == '\t' || *fnws == '\n') {
              ++fnws;
              --len;
          } else break;
      }
      
      if (fnws > &buf[0]) {
          memmove(&buf[0], fnws, len+1);
          p = &buf[0] + len;
          *p = '\0';
      }
  }

    void dquote_strip_left() {
        long n = len();
        if (0 == n) return;

        if (buf[0]=='\"') {
            memmove(&buf[0], &buf[1], n-1);
            --p;
        }
        *p=0;
    }
    
    void dquote_strip_right() {
        char* newp = p -1;
        if(newp > &buf[0] && ( (*newp) =='\"')) {
            (*newp) ='\0';
            newp--;
        }
        p = newp+1;
    }
    
    void dquote_strip() {
        dquote_strip_left();
        dquote_strip_right();
    }


  void trim() {
    trimr();
    triml();
  }

  void test() {
    l3str tmp(0,"   hi there kids! \n  ");
    tmp.trim();
    tmp.outln();
  }

  char* dotbasename() {
    char* newp = p -1;
    while(newp > &buf[0] && ( (*newp) != '.')) {
      newp--;
    }
    if (newp == &buf[0]) return newp;
    return newp+1;
  }

  inline int bufsz() { return str_max; }

}; // pathprint, pathprint;

typedef l3str<PATH_MAX> l3path;

typedef l3str<20000> l3bigstr;

#endif /* L3PATH_H */

