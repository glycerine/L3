#ifndef _L3_MQ_H_
#define _L3_MQ_H_

// Copyright (C) 2011 Jason E. Aten. All rights reserved.


// JudyL here, not JudySL.

// g++ -I. judyl.cpp -o jl /usr/local/lib/libjudy.a; ./jl

 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler

 #include <stdio.h>
 #include <string.h>
 #include <assert.h>
 #include <iostream> 
 #include "dv.h"
 #include "bool.h"

 #include "jlmap.h"

 #ifdef _USE_L3_PATH
 #include "l3path.h"
 #endif


//#include "autotag.h"
#include "l3obj.h"
#include "terp.h"
#include "serialfac.h"


struct Tag;

//
// we want to be able to index either by number (judyL) or by string judyS,
//  which is exactly what the l3obj is set up to do. so lets use those
//  indices to point to l3obj which represent the doubly linked lists that
//  are a part of the book. Hence we need a basic doubly linked list.
// 

struct ddqueue {
    MIXIN_TYPSER()

    // structures
    struct ll {
        ll*     _next;
        ll*     _prev;
        lnk*    _ptr;
    };

    // members

    //
    // optimisitically accelerate the common case of iterating through each entry in turn
    //  by index number 0 .. (len-1), using a single next-value cached prediction.
    //
    long  cache_plus_index; // can be negative or positive integer; this cache for positive increments
    ll*   cache_plus_index_entry; // the ll* corresponding to the cached_index number.

    long  cache_minus_index;     // this cache for negative increments
    ll*   cache_minus_index_entry;

    Tag*  _mytag;

    // We have one doubly linked list here, _fifo_head.
    ll*               _fifo_head;  // aka front. back is _fifo_head->_prev;

    long              _size;

    // for deletes / other ops that need to invalidate the index lookup cacheing mechanism
    void flush_index_cache() {
        cache_plus_index_entry  = 0;
        cache_minus_index_entry = 0;
    }

    ll* seek_by_index(long index) {

        if (_size == 0) return 0;
            
            long sign = (index >= 0) ? 1 : -1;
            long absIndex = labs(index) % _size; // don't do more work than we need to; and handle wrap around automatically.
            ll* cur = head();

            if (sign == 1) {
                for (long i = 0; i < absIndex; ++i) {
                    cur = cur->_next;
                }

            } else {
                for (long i = 0; i < absIndex; ++i) {
                    cur = cur->_prev;
                }
            }
            
            return cur;
    }


    lnk*  ith(long index, ll** phit) {

        if (_size == 0) return 0;

        ll* hit = 0;

        // check the plus / minus caches first
        if (index == cache_plus_index && cache_plus_index_entry) {
            hit = cache_plus_index_entry;
            
        } else if (index == cache_minus_index && cache_minus_index_entry) {
            hit = cache_minus_index_entry;

        }

        if (!hit) {
            // cache miss: find the entry manually and re-establish cache
            hit = seek_by_index(index);
        }

        cache_minus_index = index-1;
        cache_plus_index  = index+1;
        
        cache_plus_index_entry = hit->_next;
        cache_minus_index_entry = hit->_prev;

        if (phit) {
            *phit = hit;
        }
        return hit->_ptr;
    }


    // methods
    ddqueue(Tag* tag) : _mytag(tag) {

        Tag* tmptag = _mytag;
        bzero(this,sizeof(ddqueue));
        _mytag = tmptag;

        _ser  = tyse_tracker_add((tyse*)this);
        assert(_mytag);
        init();
    }

    // copy constructor
    ddqueue(const ddqueue& src) {
        bzero(this,sizeof(ddqueue));
        _mytag = src._mytag;
        _ser  = tyse_tracker_add((tyse*)this);
        init();
        if(src.size()) {
            push_back_all_from(src);
        }
    }

    ddqueue& operator=(const ddqueue& src) {
        if (&src == this) return *this; // no self-assignment

        clear();
        push_back_all_from(src);

        return *this;
    }
        
        ~ddqueue() {
            tyse_tracker_del((tyse*)this);
            destruct();
        }

    void init();
    void destruct();

    void    make_head(ll* ref) {
              assert(ref);
              // we are the first here, just point to ourself.
              ref->_next = ref;
              ref->_prev = ref;
              _fifo_head = ref;
    }

    // 
    // del: returns the _ptr of the deleted node.
    //
    lnk* del(ll* w) {

        if (!w) return 0;
        _size--;
        lnk* ptr = w->_ptr;
        bool emptychain = false;

        if (w->_prev == w) {
            // last ref, no chain to modify
            assert(w->_next == w);
            _fifo_head = 0;
            assert(_size == 0);
            emptychain = true;

        } else {
            // take ourselves out of the loop

            if (_fifo_head == w) {
                _fifo_head = _fifo_head->_next;
            }

            w->_prev->_next = w->_next;
            w->_next->_prev = w->_prev;
            
            // and isolate ourselves, to avoid any inf loops upon debug examining.
            w->_prev = w;
            w->_next = w;
        }

        ::free(w);
        w=0;
        flush_index_cache();
        return ptr;
    }


    ll*  push_back(lnk* ptr) {

        // phase 1 of 3: allocate new node for our ordered queue.
        ll* ref = (ll*)calloc(1,sizeof(ll));
        ref->_ptr = ptr;

        // phase 2 of 3: add the ref containing ptr to our ordered queue.
        // this also finishes initializing ref.
        if (_fifo_head) {
            // insert our ref into the double linked chain, last (== prior to head)
            ref->_prev = _fifo_head->_prev;
            ref->_next = _fifo_head;

            _fifo_head->_prev->_next = ref;
            _fifo_head->_prev = ref;
        } else make_head(ref);

        ++_size;       
        return ref;
    }

    // ptr becomes the node at the head of the list
    ll*  push_front(lnk* ptr) {
        // gotta allow zero as a legit value on the stack;        if (!ptr) return 0;

        // phase 1
        ll* ref = (ll*)calloc(1,sizeof(ll));
        ref->_ptr = ptr;

        // phase 2
        if (_fifo_head) {
            ref->_prev = _fifo_head->_prev;
            ref->_next = _fifo_head;

            _fifo_head->_prev->_next = ref;
            _fifo_head->_prev = ref;            

            _fifo_head = ref;
        } else make_head(ref);

        ++_size;
        return ref;
    }

    long size() const { return _size; }

    ll* front() const {
        return _fifo_head;
    }
    ll* head() const {
        return _fifo_head;
    }

    ll* back() const {
        if (0==_fifo_head) return 0;
        return _fifo_head->_prev;
    }
    ll* tail() const {
        if (0==_fifo_head) return 0;
        return _fifo_head->_prev;
    }


    ll* next(ll* ref) {
        return ref->_next;
    }

    ll* prev(ll* ref) {
        return ref->_prev;
    }

    lnk* front_val() { 
        if (!_fifo_head) return 0;
        return front()->_ptr; 
    }
    lnk* back_val()  { 
        if (!_fifo_head) return 0;
        return back()->_ptr; 
    }


    lnk* pop_back() {
        lnk* r = back_val();
        del(back());
        return r;
    }

    lnk* pop_front() {
        lnk* r = front_val();
        del(front());
        return r;
    }

    void dump(const char* indent, stopset* stoppers, const char* aft);


    void dump_to_l3path(l3path* out) {
        assert(out);
        if(0==_fifo_head) return;

        ll* cur  = head();
        ll* last = tail();
        
        while(1) {
            out->pushf("%s ",cur->_ptr); // allow multiple with space separation
            if (cur == last) break;
            cur = cur->_next;
        }
        out->chomp(); // remove the last space.

    } // end dump_to_l3path()


    void clear() {
        if (!_size) return;
        DV(printf("in ddqueue::clear() with size : %ld\n",size()));

        assert(_fifo_head);
        ll* cur  = 0;

        while(1) {
            // grab the head each time, because we might delete both cur and next
            // at once if they share the same ptr value.
            cur = head();
            if (!cur) break;

            DV(printf("in ddqueue::clear(), about to delete : %p\n",cur->_ptr));
            del(cur);
        }

        assert(_size==0);
    }

    // iterator like-iterface
    //
    // WARNING: the it interface is *not* robust against deletions. If you start iterating and then delete
    //     stuff from the list and keep iterating, you'll probably get screwed up results or crash.
    //
    struct ddqueue_it {

        ll* _cur;
        ll* _nex;
        ll* _last;
        ddqueue* _staq;
        long    _size_at_start;
        long    _numincr;
        
        ddqueue_it(ddqueue* staq = 0) {
            _staq=staq;
            // cannot restart right away, if we don't have staq yet
            if (_staq) {
                restart();
            }
        }
        void set_staq(ddqueue* staq) {
            assert(staq);
            _staq=staq;
            restart();
        }

        inline void restart()
        {
            assert(_staq);
            _cur  = _staq->head();
            if (_cur) { 
                _nex  = _cur->_next; 
            } else {
                _nex = 0;
            }
            _last = _staq->tail();
            
            _size_at_start = _staq->size();
            _numincr = 0;
        }
        
        inline ll* operator()() { 
            assert(_staq);
            return _cur; 
        }
        inline lnk* operator*()  { 
            assert(_staq);
            return _cur->_ptr; 
        }

        
        inline bool       at_end() { 
            assert(_staq);
            return (_numincr == _size_at_start); 
        }
        
        inline void operator++() {
            assert(_staq);
            if (at_end()) {
                assert(_cur == _last);
                return; // we are done
            }
            _numincr++;
            
            _cur = _nex;
            _nex = _cur->_next;
        }
    };
    
    // a default iterator already instantiated, for ease of use.
    ddqueue_it  it;

    void push_back_all_from(const ddqueue& src) {
        ddqueue& nsrc = (ddqueue&)src;
        for (nsrc.it.restart(); !nsrc.it.at_end(); ++nsrc.it ) {
            push_back(*nsrc.it);
        }
    }

};



#endif /* _L3_MQ_H_ */

