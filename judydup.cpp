//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

#include "dv.h"
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>


#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <math.h>
#include <openssl/sha.h>
#include "autotag.h"
#include "l3obj.h"
//#include "minifs.h"
#include "objects.h"
#include "terp.h"

#include "judydup.h"
#include <dlfcn.h>
#include "ut.h"
#include "tostring.h"
#include <stdio.h>
#include <editline/readline.h>
#include "dstaq.h"

#include "Judy.h"
#include "xcep.h"

/////////////////////// end of includes


// new name : judydup.cpp
// orig name: judy1dup.c
// jea: modify to use the judyL and judyS routines instead of Judy1

/*******************************************************************
 * Name: Judy1Dup
 *
 * Description:
 *       Clone (duplicate) a Judy Array.
 *
 * Parameters:
 *       PPvoid_t PPDest (OUT)
 *            Pointer to a new Judy array with the same
 *            index/value pairs as PSource.
 *            Any initial value pointed to by PPDest is ignored.
 *
 *       Pvoid_t PSource (IN)
 *            Ptr to source Judy array being duplicated.
 *            If PSource is NULL, TRUE is returned since this
 *            is simply a valid Null Judy array.
 *
 *       JError_t *PJError (OUT)
 *            Judy error structure pointer.
 *
 * Returns:
 *       JERR - error, see PJError
 *      !JERR - success
 */

int
Judy1Dup(PPvoid_t PPDest, Pvoid_t PSource, JError_t * PJError)
{
    Pvoid_t   newJArray = 0;            // new Judy1 array to ppopulate
    Word_t    kindex;                   // Key/index
    int       Ins_rv = 0;               // Insert return value

    for (kindex = 0L, Ins_rv = Judy1First(PSource, &kindex, PJError);
         Ins_rv == 1; Ins_rv = Judy1Next(PSource, &kindex, PJError))
    {
        Ins_rv = Judy1Set(&newJArray, kindex, PJError);
    }
    if (Ins_rv == JERR)
        return Ins_rv;

    *PPDest = newJArray;
    return Ins_rv;
}                                       /*  Judy1Dup */


/* sample use, from examples/Judy1DupCheck.c

    static Word_t knowns[] = { 0, 1, 1024, 4095, 4096, 4097, 4098, 123456 };
    int       i;
    Pvoid_t   PJArray = 0;
    Pvoid_t   PJArrayNew = 0;
    Word_t    Index;
    int       Judy_rv;                  // Judy Return value
    JError_t  JError;

    //  populate a judy array with known values
    PJArray = ularray2Judy(knowns, LARRAYSIZE(knowns));

    printf("Testing Judy1Dup ...");

    // dup the judy array
    if ((Judy1Dup(&PJArrayNew, PJArray, &JError)) == JERR)
    {
        printf("Judy1Dup failed: error %d\n", JU_ERRNO(&JError));
        return (2);
    }
*/

#if 0
int
JudyLDup(PPvoid_t PPDest, Pvoid_t PSource, JError_t * PJError)
{
    Pvoid_t   newJArray = 0;            // new JudyL array to ppopulate
    Word_t    kindex;                   // Key/index
    int       Ins_rv = 0;               // Insert return value

    for (kindex = 0L, Ins_rv = JudyLFirst(PSource, &kindex, PJError);
         Ins_rv == 1; Ins_rv = JudyLNext(PSource, &kindex, PJError))
    {
        Ins_rv = JudyLSet(&newJArray, kindex, PJError);
    }
    if (Ins_rv == JERR)
        return Ins_rv;

    *PPDest = newJArray;
    return Ins_rv;
}                                       /*  JudyLDup */
#endif

int copy_judyL(void* src, void** dest)  {

  /*
        JLI(PValue, PJLArray, Index) // JudyLIns()
                      Insert an Index and Value into the JudyL array PJLArray.
                      If the Index is successfully inserted, the Value is ini‐
                      tialized  to  0.  If  the Index was already present, the
                      Value is not modified.

                      Return PValue pointing to Value.  Your program  can  use
                      this  pointer  to  read  or  modify Value until the next
                      JLI() (insert), JLD() (delete) or JLFA() (freearray)  is
                      executed on PJLArray. Examples:

                      *PValue = 1234;
                      Value = *PValue;

                      Return  PValue  set to PJERR if a malloc() fail occured.
  */

    *dest = 0; // default value, if we don't finish
    Pvoid_t   newJArray = 0;         // new JudyL array to ppopulate
    Word_t   Index  = 0;                     // array index
    //Word_t   Value  = 0;                     // array element value
    Word_t * PValue = 0;                // pointer to array element value
    Word_t * PValueIns = 0;                // pointer to array element value
    //    int      Rc_int = 0;                    // return code

    volatile int complete = 0;
    complete = 0;

    //Word_t*  DestPValue = 0;

    XTRY
     case XCODE:

     JLF(PValue, src, Index);
     if (PValue == PJERR) { l3throw(XARRAYDUP_FAILURE); }

     while (PValue != NULL)
      {
	DV(printf("%lu   -> %.6f\n", Index, *((double*)PValue)));
	JLI(PValueIns, newJArray, Index);
	if (PValueIns == PJERR) { l3throw(XARRAYDUP_FAILURE); }
	*PValueIns = *PValue;
	JLN(PValue, src, Index);

	if (PValue == NULL) {
	  complete = 1;
	  break;
	}
      }

     break;
     case XFINALLY:
       if (complete) {
	 *dest = newJArray;
       }
      break;
    XENDX

   return 0;
}


int copy_judySL(void* src, void** dest)  {

    *dest = 0; // default value, if we don't finish
    Pvoid_t   newJArray = 0;         // new JudyL array to ppopulate

    Word_t * PValue = 0;                // pointer to array element value
    Word_t * PValueIns = 0;                // pointer to array element value

    volatile int complete = 0;
    complete = 0;

    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.

    XTRY
     case XCODE:

     JSLF(PValue, src, Index);
     if (PValue == PJERR) { l3throw(XARRAYDUP_FAILURE); }

     while (PValue != NULL)
      {
	DV(printf("%s   -> %.06f\n", Index, *((double*)PValue)));
	JSLI(PValueIns, newJArray, Index);
	if (PValueIns == PJERR) { l3throw(XARRAYDUP_FAILURE); }
	*PValueIns = *PValue;
	JSLN(PValue, src, Index);

	if (PValue == NULL) {
	  complete = 1;
	  break;
	}
      }

     break;
     case XFINALLY:
       if (complete) {
	 *dest = newJArray;
       }
      break;
    XENDX

   return 0;
}


int copy_judySL_typed(judys_llref* src, judys_llref** dest) {

    // use the assignment operator
    **dest = *src;

#if 0
    *dest = 0; // default value, if we don't finish
    Pvoid_t   newJArray = 0;         // new JudyL array to ppopulate

    Word_t * PValue = 0;                // pointer to array element value
    Word_t * PValueIns = 0;                // pointer to array element value

    volatile int complete = 0;
    complete = 0;

    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.

    XTRY
     case XCODE:

     JSLF(PValue, src, Index);
     if (PValue == PJERR) { l3throw(XARRAYDUP_FAILURE); }

     while (PValue != NULL)
      {
          DV(printf("%s   -> %.06f\n", Index, *((double*)PValue)));
          JSLI(PValueIns, newJArray, Index);
          if (PValueIns == PJERR) { l3throw(XARRAYDUP_FAILURE); }
          *PValueIns = *PValue;
          JSLN(PValue, src, Index);
          
          if (PValue == NULL) {
              complete = 1;
              break;
          }
      }

     break;
     case XFINALLY:
         if (complete) {
             *dest = newJArray;
         }
         break;
    XENDX

#endif

   return 0;

}


#if 0 // insert() usage out of date, we've gone to add_alias now
void test_judySL() {

  l3obj* o = 0;
  make_new_obj("testclass","testclastobjname", defptag_get(),0,&o);

  l3path name("tester");
  insert(&name, 0, main_env);

  llref* llr = (llref*)lookup_hashtable(main_env, name()); // can return 0.
  if (llr) {
      l3obj* found = llr->_obj;
      print(found,"",0);
  }

}
#endif // 0

