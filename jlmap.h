//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef _L3_JUDYL_MAP_H_
#define _L3_JUDYL_MAP_H_


// JudyL here, not JudySL.

// g++ -I. judyl.cpp -o jl /usr/local/lib/libjudy.a; ./jl

 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler

 #include <stdio.h>
 #include <string.h>
 #include <assert.h>
 #include <iostream> 
 #include "dv.h"
 #include "tyse.h"
 #include "ostate.h"
 #include "bool.h"

typedef const char* t_typ; // refer to one of the below


//void* tyse_malloc(t_typ ty, size_t size, int zerome = 0);
//void  tyse_free(void *ptr);


 #ifdef _USE_L3_PATH
 #include "l3path.h"
 #endif

// #include "judyl.h"


//  judyLmap :
//
// <K,V> parameterized version of JudyL map, for key/value type safety
// 

// global not_avail singleton -- some valid place to point
extern char* notavail_global_jlmap_not_available;

//  clients should do this once, as a global:
// char* notavail_global_jlmap_not_available = (char*)"notavail_global_jlmap_not_available";

template <typename K, typename V>
struct jlmap {
    MIXIN_TYPSER()

    void*  _judyL;

    long size() {
        Word_t    judyl_size = 0;
        JLC(judyl_size, _judyL, 0, -1);
        return judyl_size;
    }

    jlmap() {
        init();
    }
    void init() {
        _judyL=0;
    }
    
    ~jlmap() {
        assert(!_judyL); // better be cleaned up manually!
    }

    void dump(const char* indent = "") {
        judyLdump(this, indent);
    }

};




// iterator like-iterface
template <typename K, typename V>
struct jlmap_it {
    K              _key;
    Word_t         _wIndex;
    PWord_t        _pwPValue; // has to be this type coming out.
    V*             _PValue; 
    jlmap<K,V>*    _pmap;
    V              _NA;
    
    jlmap_it(jlmap<K,V>* map) {
        init(map);
    }

    void init(jlmap<K,V>* map) {
        assert(map);
        _PValue=0;
        _pwPValue=0;
        _pmap=map;

        // get first key/val pair into Index/PValue
        restart();
    }

    inline void restart()
    {
        _wIndex=0; // requests first value
        _pwPValue = 0;

        JLF(_pwPValue, _pmap->_judyL, _wIndex);
        if (_pwPValue) {
            _key = (K)_wIndex;
            _PValue = (V*)_pwPValue;
        } else {
            _key = 0;

            // avoid having to test for _PValue == 0 every time, by always having *_PValue valid address; even if
            // not meaningful to the client.
            _PValue = (V*)notavail_global_jlmap_not_available;
        }
    }
    
    // commented out to
    // avoid confusion with operator() on the ustaq_it which returns something different than operator*
    //    inline V operator()() { 
    //        return *_PValue; 
    //    }
    inline V operator*()  { 
        return *_PValue;
    }

    inline K          key() { return _key; }
    inline V          val() { return *_PValue; }

    // we need a sentinel value to indicate no more
    //  that is: char* notavail_global_jlmap_not_available;
    //
    inline bool       at_end() { return _PValue == (V*)(notavail_global_jlmap_not_available); }
    
    inline void operator++() {
        // get next key/val pair into Index/PValue
        JLN(_pwPValue, _pmap->_judyL, _wIndex);
        if (_pwPValue != 0) {
            _PValue = (V*)_pwPValue;
        } else {
            _PValue = (V*)(notavail_global_jlmap_not_available);
        }
        _key = (K)_wIndex;
    }
};




template <typename K, typename V>
jlmap<K,V>* newJudyLmap() {

    //    jlmap<K,V>* map = (jlmap<K,V>*)calloc(1,sizeof(jlmap<K,V>));
    jlmap<K,V>* map = (jlmap<K,V>*)tyse_malloc(t_map,sizeof(jlmap<K,V>),1);
    assert(map);

    //map->_judyL = 0; // redundant with calloc above.
    return map;
}




template <typename K, typename V>
void deleteJudyLmap(jlmap<K,V>* map) {

    assert(map);

    int Rc_word = 0;
    if (map->_judyL !=0) { 
        JLFA(Rc_word, (map->_judyL)); 
    }
    assert(map->_judyL == 0);
    //::free(map);
    tyse_free(map);
}



template <typename K, typename V>
void clear(jlmap<K,V>* map) {

    long Rc_word = 0;
    JLFA(Rc_word, map->_judyL);
    assert(map->_judyL == 0);

}


template <typename K, typename V>
long judyl_size(jlmap<K,V>* o) {
    Word_t    judyl_size = 0;
    JLC(judyl_size, o->_judyL, 0, -1);
    return judyl_size;
}


template <typename K, typename V>
void insertkey(jlmap<K,V>* o, K key, V value) {

    void**  PValue = 0; // pointer to return value
    Word_t   Index = *((Word_t*)(&key));
    
    PValue = JudyLIns(&(o->_judyL), Index, PJE0);
    //    JLI( PValue, o->_judyL, Index);

    if ((long)PValue == -1) {
        // what to do???
        printf("error in jmap.h  insertkey(jlmap, K, V).\n");
    }
    
    (*((V*)PValue)) = value;

    /* working example:
      l3obj* double_set(l3obj* obj, long i, double d) {
    LIVEO(obj);
    double*   pd = 0;
    PWord_t   PValue = 0;
    JLI(PValue, obj->_judyL, i);

    pd = (double*)(PValue);
    *pd = d;

    */


}



template <typename K, typename V>
void judyLdump(jlmap<K,V>* o, const char* indent = "") {

    size_t   sz = 0;
    PWord_t  pwPValue = 0;  // type checkable return value (grr)
    V*       PValue = 0;  // return value
    Word_t   Index = 0;

    // documentation, from "man JudyLNext"
    // 
    //         PPvoid_t JudyLFirst(     Pcvoid_t  PJLArray, Word_t * PIndex, PJError_t PJError);
    //
    //        JudyLFirst(PJLArray, &Index, &JError)
    //
    //                      #define JLF(PValue, PJLArray, Index)       
    //                         PValue = JudyLFirst(PJLArray, &Index, PJEO)

    JLF(pwPValue,  *((Pcvoid_t*)(&(o->_judyL))), (Index));
    PValue = (V*)(pwPValue);

    long count = judyl_size(o);
    printf("\n ****** begin judyLdump\n");
    printf("judyl_size() returned size of %ld\n",count);

    while (PValue != NULL)
    {
      V ele = *((V*)(PValue));
      printf("%s%02ld:%ld -> %p\n", indent, sz, Index, (void*)ele);

      JLN(pwPValue, o->_judyL, Index);   // get next string
      PValue = (V*)(pwPValue);
      ++sz;
    }

    printf("\n ****** end judyLdump\n");

}


// for key: Word_t same as void*, right?
//
template <typename K, typename V>
V lookupkey(jlmap<K,V>* o, K key)
{
    void**   PValue  = 0;                    // Judy array element.
    Word_t    index  = *((Word_t*)(&key));
    assert(o);
    // no, the _judyL could still be empty if we've put nothing in it, so we cannot say: assert(o->_judyL);

    //    JLG(PValue, o->_judyL,index);       // get
    PValue = JudyLGet((Pvoid_t)(o->_judyL), index, PJE0);

    // error check:
    if ((long)PValue==-1) {
        assert(0);
    }
    if (PValue)  return (*((V*)PValue));
    return 0;
}


template <typename K, typename V>
void deletekey(jlmap<K,V>* o, K key)
{
  int Rc=0;
  Word_t* pkey = (Word_t*)&key;
  JLD(Rc,o->_judyL, (*pkey));
  assert(Rc != JERR);  assert(Rc == 1); // assert it was found i.e. Rc == 1.
}





// ustaq: a stack of unique pointers (no duplicate pointers allowed, in paticular
//  because the underlying JudyL index does not allowed duplicate keys).
// 
// The ustaq structure preserves the order of addition (push_front / push_back) maintained,
//  and so can be used as a LIFO or FIFO queue/stack.
//
// ustaq is also ppropriate as a set (e.g. std::set replacement) since the the
//   JudyL index provides fast membership testing for a given pointer value. A hashset
//   may or may not be faster.
//
// ustaq: double linked list, with index by value stored.
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

// in ostate.h : typedef enum { O_DEAD=0, O_ALIVE=1  } O_STATE;

template <typename T>
struct ustaq {
    MIXIN_TYPSER()

    // structures
    struct ll {
        MIXIN_TYPSER()
    
        ll*     _next;
        ll*     _prev;
        T*      _ptr;
    };

    // members
    ll*               _head;  // aka front. back is _head->_prev;
    jlmap<T*, ll* >*  _jlmap; // for fast lookup and delete.
    long              _size;

    // feature: auto-cleanup of members; for simplicity.
    typedef enum { cln_nothing=0, cln_delete=1, cln_free=2, cln_tyse_free=3 } CLEANUP_STYLE;
    CLEANUP_STYLE     _cln;
    
    CLEANUP_STYLE     cleanstyle() { return _cln; }
    void              cleanstyle_set(CLEANUP_STYLE cln) { _cln = cln; }

    void cleanup_contained_and_clear() {
        if (_size==0) return;
        if (_cln == cln_nothing) return;
        it.set_staq(this);
        if (_cln == cln_delete) {
            for (; it.at_end(); ++it) {
                assert(0); // unimplemented for now, since g++ complains:jlmap.h:358: warning: deleting ‘void*’ is undefined
                //                delete ((T*)(*it));
            }
        } else if (_cln == cln_free) {
            for (; it.at_end(); ++it) {
                ::free((T*)(*it));
            }
        } else if (_cln == cln_tyse_free) {
            for (; it.at_end(); ++it) {
                tyse_free((T*)*it);
            }
        }
        clear();
    }

    // methods
    ustaq() {
        init();
    }

    // copy constructor
    ustaq(const ustaq& src) {
        init();
        if(src.size()) {
            push_back_all_from(src);
        }
    }

    ustaq& operator=(const ustaq& src) {
        if (&src == this) return *this; // no self-assignment

        clear();
        push_back_all_from(src);

        return *this;
    }

    // we must use destructors...so: if something needs deleting in an exception/throw situation, use newdel_push on the owning tag.
    ~ustaq() {
        if (_cln != cln_nothing) { cleanup_contained_and_clear(); }

         if (_jlmap) { destruct(); }

         tyse_tracker_del((tyse*)this);
    }

    void init() {
        // TYPSER initialization:
        _type = t_ust;
        _ser  = tyse_tracker_add((tyse*)this);

        _cln = cln_nothing;
        _head = 0;
        _size = 0;
        _jlmap = newJudyLmap<T*,ll* >();
        assert(_jlmap);
        assert(_jlmap->_judyL == 0);
        it.set_staq(this);
    }

    void destruct() {

        clear();
        deleteJudyLmap(_jlmap);
        _jlmap = 0;
    }

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
        return lookupkey(_jlmap, ptr);
    }
    ll*    member(const T* ptr) {
        assert(ptr);
        assert(_jlmap);
        return lookupkey(_jlmap, (T*)ptr);
    }


    void dup_check(T* ptr) {
        if (!ptr) return;
        if (member(ptr)) {
            printf("error in jlmap.h: ustaq::push_back(): duplicates not allowed but one just detected upon attempted insert of pointer: %p\n",
                   (void*)ptr);
            assert(0);
            exit(1);
        }
    }

    ll*  push_back(T* ptr) {
        if (!ptr) return 0;
        dup_check(ptr);

        //        ll* ref = (ll*)calloc(1,sizeof(ll));
        ll* ref = (ll*)tyse_malloc(t_ull,sizeof(ll),1);
        ref->_ptr = ptr;

        insertkey(_jlmap, ptr, ref);
        
        if (_head) {
            // insert our ref into the double linked chain, last (== prior to head)
            ref->_prev = _head->_prev;
            ref->_next = _head;

            _head->_prev->_next = ref;
            _head->_prev = ref;
        } else make_head(ref);

        ++_size;
        return ref;
    }

    // const: the biggest waste of time, ever, when after-the-fact bolted on.
    ll*  push_back(const T* ptr) { return push_back((T*)ptr); }
    ll*  push_front(const T* ptr) { return push_front((T*)ptr); }


    // ptr becomes the node at the head of the list
    ll*  push_front(T* ptr) {
        if (!ptr) return 0;
        dup_check(ptr);

        //        ll* ref = (ll*)calloc(1,sizeof(ll));
        ll* ref = (ll*)tyse_malloc(t_ull,sizeof(ll),1);
        ref->_ptr = ptr;

        insertkey(_jlmap, ptr, ref);

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
    T* del(ll* w) {
        assert(size());

        if (!w) return 0;
        _size--;
        T* ptr = w->_ptr;

        if (w->_prev == w) {
            // last ref, no chain to modify
            assert(w->_next == w);
            _head = 0;
            assert(_size == 0);
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

        deletekey(_jlmap, w->_ptr);
        //        ::free(w);
        tyse_free(w);
        w=0;

        if(_jlmap->size() != _size) {
            printf("internal error in jlmap.h del() : _jlmap->size()=%ld   did not agree with internal _size=%ld\n",
                   _jlmap->size(), _size);

            assert(_jlmap->size() == _size);
        }
        return ptr;
    }

    T* del_val(T* ptr) {
        if (!ptr) return 0;
        ll* ref = lookupkey(_jlmap, ptr);
        return del(ref);
    }

    T* pop_back() {
        assert(size());
        T* r = back_val();
        del(back());
        return r;
    }

    T* pop_front() {
        assert(size());
        T* r = front_val();
        del(front());
        return r;
    }

    void dump(const char* indent = "", bool print_tyse = false) {
        std::cout << indent << ">>>>>>>>>> begin ustaq::dump()\n";
        tyse* t = 0;

        if(0==_head) {
            std::cout << "empty" << std::endl;
        } else {

            //            std::cout << std::endl;
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                std::cout << indent << (cur->_ptr) ;

                if (print_tyse) {
                    t = (tyse*)(cur->_ptr);
                    std::cout << *t;
                }

                std::cout << std::endl;            
                if (cur == last) break;
                cur = cur->_next;
            }
            
        }
        std::cout << indent << ">>>>>>>>>> end of ustaq::dump()\n";

    } // end dump()

    void dump_tyse(const char* indent = "") {
        dump(indent,true);
    }

    void dump_char(const char* indent = "") {
        std::cout << indent << ">>>>>>>>>> begin ustaq::dump()\n";

        if(0==_head) {
            std::cout << "empty" << std::endl;
        } else {

            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                std::cout << indent << (cur->_ptr) << " : " << (char*)(cur->_ptr) << std::endl;
                if (cur == last) break;
                cur = cur->_next;
            }
            
        }
        std::cout << indent << ">>>>>>>>>> end of ustaq::dump()\n";

    } // end dump_char()



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
        DV(printf("in ustaq::clear() with size : %ld\n",size()));

        assert(_head);
        assert(size());
        ll* cur  = 0;
        while(1) {
            if (0==size()) break;
            cur = head();
            assert(cur);
            del(cur);
        }

    }

    // iterator like-iterface
    struct ustaq_it {

        ll* _cur;
        ll* _nex;
        ll* _last;
        ustaq<T>* _staq;
        long    _size_at_start;
        long    _numincr;
        
        ustaq_it(ustaq<T>* staq = 0) {
            _staq=staq;
            // cannot restart right away, if we don't have staq yet
            if (_staq) {
                restart();
            }
        }
        void set_staq(ustaq<T>* staq) {
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
    ustaq_it  it;

    void push_back_all_from(const ustaq& src) {
        ustaq& nsrc = (ustaq&)src;
        for (nsrc.it.restart(); !nsrc.it.at_end(); ++nsrc.it ) {
            push_back(*nsrc.it);
        }
    }

};




// judySmap - so we can lookup strings quickly...for the quicktype system especially.
//  the key is always a char* with a judySmap. The value V is type parameterized.
//

const long MAXKEY=BUFSIZ;

template<typename V>
struct judySmap {

    void* _judyS;
    long  _size;
    uint8_t*  _lastkey;


    judySmap() {
        _judyS = 0;
        _size = 0;
        _lastkey = (uint8_t*)::calloc(MAXKEY,1);
    }

    ~judySmap() {
        //free_values_and_clear();
        clear();
        ::free(_lastkey);
    }

    judySmap(const judySmap& src) {
        CopyFrom(src);
    }

    judySmap& operator=(const judySmap& src) {
        if (&src != this) {
            CopyFrom(src);
        }
        return *this;
    }

    void CopyFrom(const judySmap& src) {

        _size = src._size;
        _lastkey = (uint8_t*)strdup((char*)src._lastkey);

        if (0==_size) return;

        Word_t*  PValue = 0;                // pointer to array element value
        char      Index[MAXKEY];            // string to sort.
        
        Index[0] = '\0';                    // start with smallest string.
        
        JSLF(PValue, src._judyS, (uint8_t*)Index);
        assert(PValue != PJERR);
        
        while (PValue != NULL) {
            insertkey((char*)Index, (V)(*PValue));
            JSLN(PValue, src._judyS, (uint8_t*)Index);
        }
    }

    long size() {
        //        Word_t    judysl_size = 0;
        //        JLC(judysl_size, _judyS, 0, -1);
        //        printf("JLC() invoked on judyS size()\n");
        //        assert(_size == judysl_size);
        //        return judysl_size;
        return _size;
    }

    void clear() {
        int Rc_word = 0;
        JSLFA(Rc_word, _judyS);
        assert(_judyS == 0);
        _size = 0;
    }



    BOOL first(l3path* key, V* value) {
        if (0==_size) return FALSE;
        assert(key);
        assert(value);
        
        PWord_t      PValue = 0;                   // Judy array element.
        _lastkey[0] = '\0';                    // start with smallest string.
        
        JSLF(PValue, _judyS, _lastkey); 
        if(PValue != NULL) {
            *value = *((V*)(PValue));
            key->pushf("%s",_lastkey);
            return TRUE;
        } else {
            return FALSE;
        }
    }
    
    
    BOOL next(l3path* key, V* value) {
        if (0==_size) return FALSE;
        assert(key);
        assert(value);
        
        PWord_t      PValue = 0;                   // Judy array element.
        
        JSLN(PValue, _judyS, _lastkey); 
        if(PValue != NULL) {
            *value = *((V*)(PValue));
            key->pushf("%s",_lastkey);
            return TRUE;
        } else {
            return FALSE;
        }
    }
    

    // everything will dangle after this; best be called only by clear();
    void free_values_and_clear() {

        size_t       sz = 0;
        PWord_t      PValue = 0;                   // Judy array element.
        uint8_t      Index[BUFSIZ];            // string to sort.
        int          Rc_int;

        Index[0] = '\0';                    // start with smallest string.
        JSLF(PValue, _judyS, Index);       // get first string

        //        DV(std::cout << "judySmap::free_contents(): has size of %ld and contents:\n" << size() << std::endl);

        while (PValue != NULL)
            {
                Rc_int = 0;
                // unused now that we don't print: V ele = *((V*)(PValue));
                //DV(std::cout << "    " << Index << " -> " << ele  << std::endl);

                JSLD(Rc_int, _judyS, Index); // JudySLDel()
                assert(Rc_int == 1); // successful delete
                JSLN(PValue, _judyS, Index);   // get next string
                ++sz;
            }

        clear();
    }


    void insertkey(const char* key, V value)
    {
        
        PWord_t    PValue = 0;
        uint8_t      Index[BUFSIZ];            // string to sort.
        strncpy((char*)Index,key,BUFSIZ);
        
        JSLI(
             PValue, 
             (_judyS), 
             Index);   // store string into array
        if (0==PValue) {
            assert(0);
        }
        if ((long)PValue == -1) {
            assert(0);
        }
        
        // was here to diagnose duplicates t_typ's
        // but we never want to overwrite old string without free-ing them first,
        // so leave this insert in.
        if (*PValue != 0) {
            assert(0);
        }

        ++_size;
        (*((V*)PValue)) = value;
    }


    void dump(const char* indent = "") {

        size_t       sz = 0;
        PWord_t      PValue = 0;                   // Judy array element.
        uint8_t      Index[BUFSIZ];            // string to sort.

        Index[0] = '\0';                    // start with smallest string.
        JSLF(PValue, _judyS, Index);       // get first string

        std::cout << "judySmap " << (void*)this  << " has size of " << size() << " and contents:\n" << std::endl;

        while (PValue != NULL)
            {
                V ele = *((V*)(PValue));
                std::cout << indent << Index << " -> " << ele  << std::endl;

                JSLN(PValue, _judyS, Index);   // get next string
                ++sz;
            }
    }


    V lookupkey(const char* key)
    {
        if (_size == 0) return 0;
        PWord_t   PValue  = 0;                    // Judy array element.
        JSLG(PValue, _judyS, (uint8_t*)key);
        if (PValue)  return  (*((V*)PValue));
        return 0;
    }


    void deletekey(const char* key)
    {
        if (_size==0) return;
        int Rc=0;
        JSLD(Rc,_judyS, (uint8_t*)key);       // delete string
        assert(Rc != JERR);
        if (Rc == 1) {--_size;}
    }

};




// ddlist : doubly linked list, dupliates allowed, no fast index.


// ddlist: a stack of possibly duplicated pointers
// 
// The ddlist structure preserves the order of addition (push_front / push_back) maintained,
//  and so can be used as a LIFO or FIFO queue/stack.
//
// ddlist: double linked list.
//
// example sample use:
/*
    ddlist<long> b;
    long l823 = 823;

    b.push_back(&l823);
    printf("after push_front &l823: \n");

    // enumerate contents:
    for(b.it.restart(); !b.it.at_end(); ++b.it) { printf("next is: %ld\n", *(*b.it)); }
*/

#undef ALLOW_ZERO_ON_DDLIST 

template <typename T>
struct ddlist {
    MIXIN_TYPSER()

    // structures
    struct dd {
        MIXIN_TYPSER()
    
        dd*     _next;
        dd*     _prev;
        T*      _ptr;
    };

    // members
    dd*               _head;  // aka front. back is _head->_prev;
    long              _size;

    // methods
    ddlist() {
        init();
    }

    // copy constructor
    ddlist(const ddlist& src) {
        init();
        if(src.size()) {
            push_back_all_from(src);
        }
    }

    ddlist& operator=(const ddlist& src) {
        if (&src == this) return *this; // no self-assignment

        clear();
        push_back_all_from(src);

        return *this;
    }


    void init() {

        _head = 0;
        _size = 0;
        it.set_staq(this);
    }

    ~ddlist() {
        clear();
    }

    

    void    make_head(dd* ref) {
              assert(ref);
              // we are the first here, just point to ourself.
              ref->_next = ref;
              ref->_prev = ref;
              _head = ref;
    }

#if 1 // now implemented *without judyLmap*, via linear search.

    // return non-zero if ptr is a member of the set/queue... else 0
    dd*    member(T* ptr) {
        assert(ptr);
        #if ALLOW_ZERO_ON_DDLIST
            if (ptr ==0) return 0;
        #endif

        ddlist_it it(this);
        for (; !it.at_end(); ++it) {
            if (*it == ptr) return it();
        }
        return 0;
    }

    dd*    member(const T* ptr) {
        assert(ptr);
        #if ALLOW_ZERO_ON_DDLIST
            if (ptr ==0) return 0;
        #endif

        ddlist_it it(this);
        for (; !it.at_end(); ++it) {
            if (*it == ptr) return it();
        }
        return 0;
    }


#endif

    dd*  push_back(T* ptr) {        

        #if ALLOW_ZERO_ON_DDLIST
            if (ptr ==0) return 0;
        #endif

        //   dd* ref = (dd*)calloc(1,sizeof(dd));
        dd* ref = (dd*)tyse_malloc(t_ddr,sizeof(dd),1);

        ref->_ptr = ptr;

        if (_head) {
            // insert our ref into the double linked chain, last (== prior to head)
            ref->_prev = _head->_prev;
            ref->_next = _head;

            _head->_prev->_next = ref;
            _head->_prev = ref;
        } else make_head(ref);

        ++_size;
        return ref;
    }

    // const: the biggest waste of time, ever, when after-the-fact bolted on.
    dd*  push_back(const T* ptr) { return push_back((T*)ptr); }
    dd*  push_front(const T* ptr) { return push_front((T*)ptr); }


    // ptr becomes the node at the head of the list
    dd*  push_front(T* ptr) {
        assert(ptr);
        #if ALLOW_ZERO_ON_DDLIST
            if (ptr ==0) return 0;
        #endif

        //        dd* ref = (dd*)calloc(1,sizeof(dd));
        dd* ref = (dd*)tyse_malloc(t_ddr,sizeof(dd),1);
        ref->_ptr = ptr;

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

    dd* front() const {
        return _head;
    }
    dd* head() const {
        return _head;
    }

    dd* back() const {
        if (0==_head) return 0;
        return _head->_prev;
    }
    dd* tail() const {
        if (0==_head) return 0;
        return _head->_prev;
    }


    dd* next(dd* ref) {
        return ref->_next;
    }

    dd* prev(dd* ref) {
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
    T* del(dd* w) {
        if (!w) return 0;
        _size--;
        T* ptr = w->_ptr;

        if (w->_prev == w) {
            // last ref, no chain to modify
            assert(w->_next == w);
            _head = 0;
            assert(_size == 0);
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

        // if it is pointing at us, move 

        //        ::free(w);
        tyse_free(w);
        w=0;

        return ptr;
    }

#if 1
    T* del_val(T* ptr) {
        if (!ptr) return 0;
        dd* ref = member(ptr);
        return del(ref);
    }
#endif

    T* pop_back() {
        T* r = back_val();
        del(back());
        return r;
    }

    T* pop_front() {
        T* r = front_val();
        del(front());
        return r;
    }

    void dump(const char* indent = "") {
        std::cout << "\n >>>>>>>>>> begin ddlist::dump()\n";
        if(0==_head) {
            std::cout << "empty" << std::endl;
        } else {

            std::cout << std::endl;
            dd* cur  = head();
            dd* last = tail();
            
            while(1) {
                std::cout << indent << (cur->_ptr)  << std::endl;            
                if (cur == last) break;
                cur = cur->_next;
            }
            
        }
        std::cout << " >>>>>>>>>> end of ddlist::dump()\n";

    } // end dump()

 #ifdef _USE_L3_PATH

    void dump_to_l3path(l3path* out) {
        assert(out);
        if(0==_head) return;

        dd* cur  = head();
        dd* last = tail();
        
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
        DV(printf("in ddlist::clear() with size : %ld\n",size()));

        assert(_head);
        dd* cur  = 0;

        while(1) {
            if (0==size()) break;
            cur = head();
            assert(cur);
            del(cur);
        }

    }

    // iterator like-iterface

    //
    // new iterator protocol: to be resilient against deletes we
    //  note the head when we start, and store it in _first. 
    //  But what if that head is subsequently deleted?
    //
    //
    struct ddlist_it {

        dd* _cur;
        dd* _nex;
        dd* _last;
        ddlist<T>* _staq;
        long    _size_at_start;
        long    _numincr;
        
        ddlist_it(ddlist<T>* staq = 0) {
            _staq=staq;
            // cannot restart right away, if we don't have staq yet
            if (_staq) {
                restart();
            }
        }
        void set_staq(ddlist<T>* staq) {
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
        
        inline dd* operator()() { 
            assert(_staq);
            return _cur; 
        }
        inline T* operator*()  { 
            assert(_staq);
            return _cur->_ptr; 
        }
        
        inline bool       at_end() { 
            if (!_staq) return true; // assert(_staq);
            return (_numincr == _size_at_start); 
        }
        
        inline void operator++() {
            if (!_staq) return;  // assert(_staq);
            if (at_end()) {
                assert(_cur == _last);
                return; // we are done
            }
            _numincr++;
            
            _cur = _nex;
            if (!at_end()) { _nex = _cur->_next; }
        }
    };
    
    // a default iterator already instantiated, for ease of use.
    ddlist_it  it;

    void push_back_all_from(const ddlist& src) {
        ddlist& nsrc = (ddlist&)src;
        for (nsrc.it.restart(); !nsrc.it.at_end(); ++nsrc.it ) {
            push_back(*nsrc.it);
        }
    }

};


#endif /* _L3_JUDYL_MAP_H_ */

