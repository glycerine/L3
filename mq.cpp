
#include "autotag.h"
#include "mq.h"
#include "symvec.h"

void ddqueue::init() {

    _fifo_head = 0;
    _size = 0;

    it.set_staq(this);
}

void ddqueue::destruct() {
    clear();
}

void ddqueue::dump(const char* indent, stopset* stoppers, const char* aft)
{

        DV(std::cout << indent << ">>>>>>>>>> begin ddqueue::dump()\n");
        if(0==_fifo_head) {
            printf("%sempty\n",indent);
        } else {

            printf("\n");
            ll* cur  = head();
            ll* last = tail();
            
            while(1) {
                printf("%s(lnk*)%p : lnk->name: '%s' ", indent,  cur->_ptr, cur->_ptr->name());

                if (!stoppers) {
                    print(cur->_ptr->chase(), indent, stoppers);
                } else {
                    if (!obj_in_stopset(stoppers, cur->_ptr)) {
                        stopset_push(stoppers, cur->_ptr);
                        print(cur->_ptr->chase(), indent, stoppers);
                        stopset_push(stoppers, cur->_ptr);
                    }
                }

                if (cur == last) break;
                cur = cur->_next;
            }            
        }
        DV(printf("%s>>>>>>>>>> end of ddqueue::dump()\n",indent));
    
} // end dump()



