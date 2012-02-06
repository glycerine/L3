#ifndef TYSE_TRACKER_H
#define TYSE_TRACKER_H

#include <stdlib.h>
#include <map>
#include "quicktype.h"
#include <ext/malloc_allocator.h>

//typedef  std::map<tyse*,long>    gtyseset;
typedef  std::map<tyse*, long >    gtyseset;
typedef  gtyseset::iterator       gtysesetit;

long  tyse_tracker_add(tyse* addme);
void  tyse_tracker_del(tyse* delme);

extern long        global_tyse_last_sn;

// type/serial tracking memory management functions

void* tyse_malloc(t_typ ty, size_t size, int zerome = 0);
void  tyse_free(void *ptr);


#if 0 // un-implemented

     void *     tyse_realloc(t_typ ty, void *ptr, size_t size);

     void *     tyse_reallocf(t_typ ty, void *ptr, size_t size);

     void *     tyse_valloc(t_typ ty, size_t size);
#endif

#endif /*  TYSE_TRACKER_H */
