#include "tyse_tracker.h"
#include "l3path.h"
#include "jmemlogger.h"
#include <map>
#include <assert.h>

/////////////////////// end of includes

// track memory for anything with a MIXIN_TYPSER

extern jmemlogger* mlog;

// clients gotta decalre a global  gtyseset    global_tyse_set; before 
//  any other allocations can be tracked.
// 
extern gtyseset*    global_tyse_set;
long        global_tyse_last_sn = 0;


long tyse_tracker_add(tyse* addme) {

    long last_tyse_ser = 0;    

    ++global_tyse_last_sn;
    last_tyse_ser = global_tyse_last_sn;

    addme->_ser = last_tyse_ser;
    global_tyse_set->insert(gtyseset::value_type(addme,last_tyse_ser));

    l3path msg(0,"111111 %p tyse added: tyse of type '%s' has @tyse_ser:%ld\n", addme, addme->_type, last_tyse_ser);
    MLOG_ADD(msg());

    return last_tyse_ser;
}



void  tyse_tracker_del(tyse* delme) {

    l3path msg(0,"222222 %p tyse delete of type '%s': had @tyse_ser:%ld\n",
               delme,  delme->_type, delme->_ser);
    MLOG_ADD(msg());

    assert(global_tyse_set->size());

    gtysesetit it = global_tyse_set->find(delme);
    if (it == global_tyse_set->end()) {
        printf("tyse_tracker_del(): delme (%p) not found! Memory corruption likely.\n",
               delme);
        assert(0);
        exit(1);
    }

    global_tyse_set->erase(it);
}


/* replacement malloc/free function functions */

#define HEADSZ sizeof(tyse)

extern quicktype_sys*  qtypesys;

void  tyse_free(void *ptr) {
    assert(ptr);

    tyse* delme = (tyse*)ptr;

    if (delme->_ser > global_tyse_last_sn) {
        printf("tyse_free(): detected corruption of %p upon free: _ser number %ld out of bounds.\n",delme,delme->_ser);
        assert(0);
    }

    long ser = delme->_ser;
    assert(ser);
    t_typ verify = qtypesys->which_type(delme->_type,0);
    if (!verify) {
        // lots of user defined types now.
        printf("tyse_free(): possible corruption of %p upon free: i.e. non-type in the _type slot '%s'.\n",delme,delme->_type);
        assert(0);
    }

    tyse_tracker_del(delme);
    ::free(delme);

#if 0 // skip pre-header stuff for now.
    tyse* delme = (tyse*)((char*)ptr - HEADSZ);
    tyse_tracker_del(delme);
    ::free(delme);
#endif
}

void* tyse_malloc(t_typ ty, size_t size, int zerome) {

    // no pre-header, make everthing directly include MIXIN_TYPSER as the first thing in the struct.
    // At least until we get this simpler form right. This is so the users's pointers correspond
    //  to what we are tracking in the log.
    //
    tyse* ret = (tyse*)::malloc(size);
    ret->_type = ty;
    long sn = tyse_tracker_add(ret);
    assert(sn == ret->_ser); // should be filled in *and* returned.

    //bzero(ret->_varname,VLEN);
    if (zerome) { 
        bzero((char*)ret + HEADSZ, size - HEADSZ); 
    }
    return ret;

#if 0 // skip the pre-header for now

    // add a pre-header with type and serial number

    tyse* ret = (tyse*)::malloc(size + HEADSZ);
    ret->_type = ty;
    long sn = tyse_tracker_add(ret);
    assert(sn == ret->_ser); // should be filled in *and* returned.

    if (zerome) { bzero(ret + HEADSZ, size); }
    return ret + HEADSZ;

#endif
}
