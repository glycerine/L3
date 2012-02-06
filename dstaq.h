#ifndef _L3_DSTAQ_H_
#define _L3_DSTAQ_H_

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

// dstaq: a stack of possibly duplicated pointers.
// 
// The dstaq structure preserves the order of addition (push_front / push_back) maintained,
//  and so can be used as a LIFO or FIFO queue/stack.
//
// dstaq is also ppropriate as a multiset (e.g. std::multi_set replacement) since the the
//   JudyL index provides fast membership testing for a given pointer value. A hashset
//   may or may not be faster.
//
// dstaq: double linked list, with index by value stored, duplicates allowed because
//   the index points to a secondary doubly linked list with entries for each duplicate.
//
// example sample use:
/*
    ustaq<long> b;
    long l823 = 823;

    b.push_back(&l823);
    printf("after push_front &l823: \n");

    // enumerate contents:
    for(b.it.restart(); !b.it.at_end(); ++b.it) { printf("next is: %ld\n", *(*b.it)); }
*/

struct Tag;

template <typename T>
struct dstaq {
    MIXIN_TYPSER()

    // structures
    struct ll {
        ll*     _next;
        ll*     _prev;
        T*      _ptr;
    };

    // We have two lists, a primary and an index list. As well, we have a map.
    //
    // The primary list is made of ll*. This is the queue, or order, of the objects.
    //
    // The secondary index list holds a list of the duplicated values in the primary, those that share
    //   the same key in the map.
    //
    // We have one _jlmap that relates the keys in the primary to the _ptr in the secondary index list.
    //
    // The _jlmap maps to an index list for each value, which
    //                              points out the duplicate values.
    //
    //  i.e.  _jlmap maps from key:T*  ->  value: primary_value_type*

    typedef T                                 primary_list_type;
    typedef ll                                primary_list_elem;

    typedef T*                                map_key_type;
    typedef ddlist<primary_list_elem>*        map_value_type_p, map_value_type;

    //
    // optimisitically accelerate the common case of iterating through each entry in turn
    //  by index number 0 .. (len-1), using a single next-value cached prediction.
    //

    long  cache_plus_index; // can be negative or positive integer; this cache for positive increments
    ll*   cache_plus_index_entry; // the ll* corresponding to the cached_index number.

    long  cache_minus_index;     // this cache for negative increments
    ll*   cache_minus_index_entry;

    Tag*  _mytag;

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


    T*  ith(long index, ll** phit) {

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


    // members
    ll*               _head;  // aka front. back is _head->_prev;

    //
    //
    // _jlmap :   _jlmap maps from T*  ->  ddlist< dstaq<T>::ll  >* 
    //
    //             for fast lookup and delete, the ddlist (map_value_type) tracks duplicate valued entries in our dstaq
    //
    jlmap<map_key_type, map_value_type_p  >*  _jlmap;

    long              _size;

    // methods
    dstaq(Tag* tag) : _mytag(tag) {

        Tag* tmptag = _mytag;
        bzero(this,sizeof(dstaq<T>));
        _mytag = tmptag;

        _ser  = tyse_tracker_add((tyse*)this);
        assert(_mytag);
        init();
    }

    // copy constructor
    dstaq(const dstaq& src) {
        bzero(this,sizeof(dstaq<T>));
        _mytag = src._mytag;
        _ser  = tyse_tracker_add((tyse*)this);
        init();
        if(src.size()) {
            push_back_all_from(src);
        }
    }

    dstaq& operator=(const dstaq& src) {
        if (&src == this) return *this; // no self-assignment

        clear();
        push_back_all_from(src);

        return *this;
    }
        
        ~dstaq() {
            tyse_tracker_del((tyse*)this);
            
            if (_jlmap) { destruct(); }
        }

    void init();
    void destruct();

#if 0 // now in dstaq.cpp
    void init() {
        _head = 0;
        _size = 0;
        _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
        _mytag->tyse_push((tyse*)_jlmap);
        assert(_jlmap);
        it.set_staq(this);
    }


    void destruct() {
        clear();
        _mytag->tyse_remove((tyse*)_jlmap);
        deleteJudyLmap(_jlmap);
        _jlmap = 0;
    }

#endif
    void    make_head(ll* ref) {
              assert(ref);
              // we are the first here, just point to ourself.
              ref->_next = ref;
              ref->_prev = ref;
              _head = ref;
    }

    // return non-zero if ptr is a member of the set/queue... else 0
    ll*    member(T* ptr) {
        assert(ptr);
        assert(_jlmap);

        map_value_type_p  p = lookupkey(_jlmap, ptr);
        if (!p) return 0;
        return p->member(ptr); // returns a dd*
    }

    ll*    member(const T* ptr) {
        assert(ptr);
        assert(_jlmap);

        map_value_type_p  p = lookupkey(_jlmap, (T*)ptr);
        if (!p) return 0;
        return p->member(ptr);
    }


    ll*  push_back(T* ptr) {        
        //        if (!ptr) return 0;

        // phase 1 of 3: allocate new node for our ordered queue.
        ll* ref = (ll*)calloc(1,sizeof(ll));
        ref->_ptr = ptr;

        // phase 2 of 3: add the ref containing ptr to our ordered queue.
        // this also finishes initializing ref.
        if (_head) {
            // insert our ref into the double linked chain, last (== prior to head)
            ref->_prev = _head->_prev;
            ref->_next = _head;

            _head->_prev->_next = ref;
            _head->_prev = ref;
        } else make_head(ref);

        ++_size;


        // phase 3 of 3: add key:ptr -> value:ref to the index.
        //
        //   old way:     insertkey(o=_jlmap, key=ptr, value=ref);

        // lookup and add to the existing ddlist if any
        // else allocate a new ddlist and put it in.

        void**  PValue = 0; // pointer to return value
        Word_t   Index = *((Word_t*)(&ptr));

        PValue = JudyLIns(&(_jlmap->_judyL), Index, PJE0);

        if (*PValue) {
            // already there, so this is a duplicate
            (*((map_value_type_p*)PValue))->push_back(ref);

        } else {
            // new key, so allocate a new ddlist;
            (*((map_value_type_p*)PValue)) = new map_value_type;
            (*((map_value_type_p*)PValue))->push_back(ref);
        }
       
        return ref;
    }

    // const: the biggest waste of time, ever, when after-the-fact bolted on.
    //    ll*  push_back(const T* ptr) { return push_back((T*)ptr); }
    //    ll*  push_front(const T* ptr) { return push_front((T*)ptr); }


    // ptr becomes the node at the head of the list
    ll*  push_front(T* ptr) {
        // gotta allow zero as a legit value on the stack;        if (!ptr) return 0;

        // phase 1
        ll* ref = (ll*)calloc(1,sizeof(ll));
        ref->_ptr = ptr;


        // phase 2 of 3: add key:ptr -> value:ref to the index.
        //
        //   old way:     insertkey(o=_jlmap, key=ptr, value=ref);

        // lookup and add to the existing ddlist if any
        // else allocate a new ddlist and put it in.

        void**  PValue = 0; // pointer to return value
        Word_t   Index = *((Word_t*)(&ptr));

        PValue = JudyLIns(&(_jlmap->_judyL) , Index, PJE0);

        if (*PValue) {
            // already there, so this is a duplicate
            (*((map_value_type_p*)PValue))->push_back(ref);
        } else {
            // new key, so allocate a new ddlist;
            (*((map_value_type_p*)PValue)) = new map_value_type;
            (*((map_value_type_p*)PValue))->push_back(ref);
        }


        // phase 3
        if (_head) {
            ref->_prev = _head->_prev;
            ref->_next = _head;

            _head->_prev->_next = ref;
            _head->_prev = ref;            

            _head = ref;
        } else make_head(ref);

        ++_size;
        return ref;
    }

    long size() const { return _size; }

    ll* front() const {
        return _head;
    }
    ll* head() const {
        return _head;
    }

    ll* back() const {
        if (0==_head) return 0;
        return _head->_prev;
    }
    ll* tail() const {
        if (0==_head) return 0;
        return _head->_prev;
    }


    ll* next(ll* ref) {
        return ref->_next;
    }

    ll* prev(ll* ref) {
        return ref->_prev;
    }

    T* front_val() { 
        if (!_head) return 0;
        return front()->_ptr; 
    }
    T* back_val()  { 
        if (!_head) return 0;
        return back()->_ptr; 
    }

    // 
    // del: returns the _ptr of the deleted node.
    //
    //
    T* del(ll* w, BOOL* pwas_last_copy) {
        assert(pwas_last_copy);
        *pwas_last_copy = FALSE; // default 

        if (!w) return 0;
        _size--;
        T* ptr = w->_ptr;
        bool emptychain = false;

        if (w->_prev == w) {
            // last ref, no chain to modify
            assert(w->_next == w);
            _head = 0;
            assert(_size == 0);
            emptychain = true;

        } else {
            // take ourselves out of the loop

            if (_head == w) {
                _head = _head->_next;
            }

            w->_prev->_next = w->_next;
            w->_next->_prev = w->_prev;
            
            // and isolate ourselves, to avoid any inf loops upon debug examining.
            w->_prev = w;
            w->_next = w;
        }

        map_value_type_p p = lookupkey(_jlmap, ptr);
        assert(p); // else how did w ever get to contain _ptr???

        ll*  goner = p->del_val(w);
        assert(goner);
        if (p->size() == 0) {
            deletekey(_jlmap, ptr); // delete the ddlist if empty

            delete p;
            *pwas_last_copy = TRUE;
        }

        ::free(w);
        w=0;
        flush_index_cache();
        return ptr;
    }

    T* del_val(T* ptr) {
        //        printf("in del_val on ptr %p : %d\n",ptr, *ptr);

        map_value_type_p p_ddlist = lookupkey(_jlmap, ptr);
        if (0==p_ddlist) return 0;

        // don't do this!  can't delete effectively like this iterator for loop, since the
        //  iterators have stale info after each delete, which valgrind
        //  reports as reading from a freed block.
        //        for (p_ddlist->it.restart(); !p_ddlist->it.at_end(); ++p_ddlist->it) {
        //            del(*(p_ddlist->it));
        //        }

        // empty the list, starting at the head() again and again
        //  until there's not even a head() node left.
        //
        // works, but opaque: primary_list_elem* ll_kill = 0;
        // clearer:
        primary_list_elem* ll_kill = 0;

        BOOL was_last_copy = FALSE;

        while(1) {
            ll_kill = p_ddlist->front_val();
            if (!ll_kill) break;

            del(ll_kill,&was_last_copy);
            if (was_last_copy) break; // del() already took care of free-ing the ddlist
        }

        flush_index_cache();
        return ptr;
    }

    T* pop_back() {
        T* r = back_val();
        BOOL was_last_copy = FALSE;
        del(back(),&was_last_copy);
        return r;
    }

    T* pop_front() {
        T* r = front_val();
        BOOL was_last_copy = FALSE;
        del(front(),&was_last_copy);
        return r;
    }

    void dump(const char* indent, stopset* stoppers, const char* aft);

 #ifdef _USE_L3_PATH

    void dump_to_l3path(l3path* out) {
        assert(out);
        if(0==_head) return;

        ll* cur  = head();
        ll* last = tail();
        
        while(1) {
            out->pushf("%s ",cur->_ptr); // allow multiple with space separation
            if (cur == last) break;
            cur = cur->_next;
        }
        out->chomp(); // remove the last space.

    } // end dump_to_l3path()

#endif // _USE_L3_PATH 

    void clear() {
        if (!_size) return;
        DV(printf("in dstaq::clear() with size : %ld\n",size()));

        assert(_head);
        ll* cur  = 0;

        while(1) {
            // grab the head each time, because we might delete both cur and next
            // at once if they share the same ptr value.
            cur = head();
            if (!cur) break;

            DV(printf("in dstaq::clear(), about to delete : %p\n",cur->_ptr));

            // do the deletion : del_val cleans up the secondary index, del does not.
            // del_val will call del repeatedly.
            del_val(cur->_ptr);
        }

        assert(_size==0);
    }

    // iterator like-iterface
    //
    // WARNING: the it interface is *not* robust against deletions. If you start iterating and then delete
    //     stuff from the list and keep iterating, you'll probably get screwed up results or crash.
    //
    struct dstaq_it {

        ll* _cur;
        ll* _nex;
        ll* _last;
        dstaq<T>* _staq;
        long    _size_at_start;
        long    _numincr;
        
        dstaq_it(dstaq<T>* staq = 0) {
            _staq=staq;
            // cannot restart right away, if we don't have staq yet
            if (_staq) {
                restart();
            }
        }
        void set_staq(dstaq<T>* staq) {
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
        inline T* operator*()  { 
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
    dstaq_it  it;

    void push_back_all_from(const dstaq& src) {
        dstaq& nsrc = (dstaq&)src;
        for (nsrc.it.restart(); !nsrc.it.at_end(); ++nsrc.it ) {
            push_back(*nsrc.it);
        }
    }

};



#if 0
std::ostream& operator<<(std::ostream& os, lnk* s);
std::ostream& operator<<(std::ostream& os, const lnk& s);
#endif

#endif /* _L3_DSTAQ_H_ */

