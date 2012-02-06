
#include "jlmap.h"
#include "autotag.h"
#include "l3obj.h"
#include "llref.h"
#include "quicktype.h"
#include "l3path.h"

//
// judySmap<llref*>  instantiations
//

template<>
void judySmap<llref*>::insertkey(const char* key, llref* value) {
        
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
        (*((llref**)PValue)) = value;
    }


// return TRUE if found something, FALSE if empty array.
template<>
BOOL judySmap<llref*>::first(l3path* key, llref** value) {
    if (0==_size) return FALSE;
    assert(key);
    assert(value);

    PWord_t      PValue = 0;                   // Judy array element.
    _lastkey[0] = '\0';                    // start with smallest string.

    JSLF(PValue, _judyS, _lastkey); 
    if(PValue != NULL) {
        *value = *((llref**)(PValue));
        LIVEREF(*value);
        key->pushf("%s",_lastkey);
        return TRUE;
    } else {
        return FALSE;
    }
}

template<>
BOOL judySmap<llref*>::next(l3path* key, llref** value) {
    if (0==_size) return FALSE;
    assert(key);
    assert(value);

    PWord_t      PValue = 0;                   // Judy array element.

    JSLN(PValue, _judyS, _lastkey); 
    if(PValue != NULL) {
        *value = *((llref**)(PValue));
        LIVEREF(*value);
        key->pushf("%s",_lastkey);
        return TRUE;
    } else {
        return FALSE;
    }
}


//
// judySmap<t_typ>  instantiations
//

    
template<>
void judySmap<t_typ>::insertkey(const char* key, t_typ value) {

        
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
        (*((t_typ*)PValue)) = value;
}



// return TRUE if found something, FALSE if empty array.
template<>
BOOL judySmap<t_typ>::first(l3path* key, t_typ* value) {
    if (0==_size) return FALSE;
    assert(key);
    assert(value);

    PWord_t      PValue = 0;                   // Judy array element.
    _lastkey[0] = '\0';                    // start with smallest string.

    JSLF(PValue, _judyS, _lastkey); 
    if(PValue != NULL) {
        *value = *((t_typ*)(PValue));
        key->pushf("%s",_lastkey);
        return TRUE;
    } else {
        return FALSE;
    }
}


template<>
BOOL judySmap<t_typ>::next(l3path* key, t_typ* value) {
    if (0==_size) return FALSE;
    assert(key);
    assert(value);

    PWord_t      PValue = 0;                   // Judy array element.

    JSLN(PValue, _judyS, _lastkey); 
    if(PValue != NULL) {
        *value = *((t_typ*)(PValue));
        key->pushf("%s",_lastkey);
        return TRUE;
    } else {
        return FALSE;
    }
}

