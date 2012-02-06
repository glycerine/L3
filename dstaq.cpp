
#include "autotag.h"
#include "dstaq.h"
#include "symvec.h"

template <typename T>
void dstaq<T>::init() {
    _head = 0;
    _size = 0;
    _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
    _mytag->tyse_push((tyse*)_jlmap);
    assert(_jlmap);
    it.set_staq(this);
}

template <typename T>
void dstaq<T>::destruct() {
    clear();
    _mytag->tyse_remove((tyse*)_jlmap);
    deleteJudyLmap(_jlmap);
    _jlmap = 0;
}


template <>
void dstaq<_tyse>::init()
 {
     _type = "dstaq<_tyse>";

    _head = 0;
    _size = 0;
    _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
    _mytag->tyse_push((tyse*)_jlmap);
    assert(_jlmap);
    it.set_staq(this);
}



template <>
void dstaq<_tyse>::destruct()
 {
    clear();
    _mytag->tyse_remove((tyse*)_jlmap);
    deleteJudyLmap(_jlmap);
    _jlmap = 0;
}



template <>
void dstaq<_symbol>::init()
 {
     _type = "dstaq<_symbol>";
    _head = 0;
    _size = 0;
    _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
    _mytag->tyse_push((tyse*)_jlmap);
    assert(_jlmap);
    it.set_staq(this);
}



template <>
void dstaq<_symbol>::destruct()
 {
    clear();
    _mytag->tyse_remove((tyse*)_jlmap);
    deleteJudyLmap(_jlmap);
    _jlmap = 0;
}




template <>
void dstaq<char>::init()
 {
     _type = "dstaq<char>";

    _head = 0;
    _size = 0;
    _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
    _mytag->tyse_push((tyse*)_jlmap);
    assert(_jlmap);
    it.set_staq(this);
}



template <>
void dstaq<char>::destruct()
 {
    clear();
    _mytag->tyse_remove((tyse*)_jlmap);
    deleteJudyLmap(_jlmap);
    _jlmap = 0;
}



template <>
void dstaq<lnk>::init()
 {
     _type = "dstaq<lnk>";

    _head = 0;
    _size = 0;
    _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
    _mytag->tyse_push((tyse*)_jlmap);
    assert(_jlmap);
    it.set_staq(this);
}



template <>
void dstaq<lnk>::destruct()
 {
    clear();
    _mytag->tyse_remove((tyse*)_jlmap);
    deleteJudyLmap(_jlmap);
    _jlmap = 0;
}



template <>
void dstaq<l3obj>::init()
 {
     _type = "dstaq<l3obj>";

    _head = 0;
    _size = 0;
    _jlmap = newJudyLmap<map_key_type, map_value_type_p>();
    _mytag->tyse_push((tyse*)_jlmap);
    assert(_jlmap);
    it.set_staq(this);
}



template <>
void dstaq<l3obj>::destruct()
 {
    clear();
    _mytag->tyse_remove((tyse*)_jlmap);
    deleteJudyLmap(_jlmap);
    _jlmap = 0;
}




#if 0
std::ostream& operator<<(std::ostream& os, lnk* s) {

    os << "(lnk*)" << (void*)s << " _name:'" <<  s->_name << "'   _target: "; // << s->_target->_varname;
    //    lnk_print(s,"",0);
    print(s->_target,"",0);
    return os;
}

std::ostream& operator<<(std::ostream& os, const lnk& s) {
    lnk_print((lnk*)&s,"",0,&os);
    return os;
}
#endif

// _tyse
// _symbol
// char
// lnk
// l3obj

template <>
void dstaq<_tyse>::dump(const char* indent, stopset* stoppers, const char* aft)
 {

        DV(std::cout << indent << ">>>>>>>>>> begin dstaq::dump()\n");
        if(0==_head) {
            printf("%sempty\n",indent);
        } else {

            printf("\n");
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                printf("%s(tyse*)%p : _type:%s  _ser:%ld\n", indent,  cur->_ptr, cur->_ptr ? cur->_ptr->_type : 0 , cur->_ptr ? cur->_ptr->_ser : 0);
                if (cur == last) break;
                cur = cur->_next;
            }            
        }
        DV(printf("%s>>>>>>>>>> end of dstaq::dump()\n",indent));

    } // end dump()



template <>
void dstaq<_symbol>::dump(const char* indent, stopset* stoppers, const char* aft)
 {

        DV(std::cout << indent << ">>>>>>>>>> begin dstaq::dump()\n");
        if(0==_head) {
            printf("%sempty\n",indent);
        } else {

            printf("\n");
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                printf("%s(_symbol*)%p\n", indent,  cur->_ptr);
                if (cur == last) break;
                cur = cur->_next;
            }            
        }
        DV(printf("%s>>>>>>>>>> end of dstaq::dump()\n",indent));


    } // end dump()


template <>
void dstaq<char>::dump(const char* indent, stopset* stoppers, const char* aft)
 {


        DV(std::cout << indent << ">>>>>>>>>> begin dstaq::dump()\n");
        if(0==_head) {
            printf("%sempty\n",indent);
        } else {

            printf("\n");
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                printf("%s%s\n", indent,  cur->_ptr);
                if (cur == last) break;
                cur = cur->_next;
            }            
        }
        DV(printf("%s>>>>>>>>>> end of dstaq::dump()\n",indent));

    } // end dump()


template <>
void dstaq<lnk>::dump(const char* indent, stopset* stoppers, const char* aft)
 {
        DV(std::cout << indent << ">>>>>>>>>> begin dstaq::dump()\n");
        if(0==_head) {
            printf("%sempty\n",indent);
        } else {

            printf("\n");
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                printf("%s(lnk*)%p : ", indent,  cur->_ptr);

                if (!stoppers) {
                    lnk_print(cur->_ptr, indent, stoppers);
                } else {
                    if (!obj_in_stopset(stoppers, cur->_ptr->target())) {
                        stopset_push(stoppers, cur->_ptr);
                        lnk_print(cur->_ptr, indent, stoppers);
                        stopset_push(stoppers, cur->_ptr->target());
                    }
                }

                if (cur == last) break;
                cur = cur->_next;
            }            
        }
        DV(printf("%s>>>>>>>>>> end of dstaq::dump()\n",indent));


    } // end dump()


template <>
void dstaq<l3obj>::dump(const char* indent, stopset* stoppers, const char* aft)
 {
        DV(std::cout << indent << ">>>>>>>>>> begin dstaq::dump()\n");
        if(0==_head) {
            printf("%sempty\n",indent);
        } else {

            printf("\n");
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                printf("%s(l3obj*)%p : ", indent,  cur->_ptr);

                if (!stoppers) {
                    print(cur->_ptr, indent, stoppers);
                } else {
                    if (!obj_in_stopset(stoppers, cur->_ptr)) {
                        stopset_push(stoppers, cur->_ptr);
                        print(cur->_ptr, indent, stoppers);
                        stopset_push(stoppers, cur->_ptr);
                    }
                }

                if (cur == last) break;
                cur = cur->_next;
            }            
        }
        DV(printf("%s>>>>>>>>>> end of dstaq::dump()\n",indent));


    } // end dump()
