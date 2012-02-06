//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>
 #include "symvec.h"

/////////////////////// end of includes

/*

// our symbols have a name and a pointer.

typedef struct _symbol {
   char   _name[40];
   l3obj* _obj;

} symbol;

*/

// symvec: a vector of symbols. each symbol is a mapping from a string to a pointer.
// 
//   The primary lookup is from an index [0], [1], [2], etc to a symbol* in the _judyL.
//      The symbol pointed to is malloced and owned by our tag. This gets us our
//      name and symbol pairing very quickly and efficiently, given an index
//      into the _judyL array.
//   
//   For lookups by symbol name, also store the names in _judyS, with each value in
//    the _judyS being a symbol* whose _name[] should be identical to the key stored
//    in the _judyS.
//
//

L3METHOD(make_new_symvec)

     l3path objname("symvec");
     l3path classname("symvec");

     l3obj* nobj = 0;
     make_new_captag((l3obj*)&objname ,0, exp,env, &nobj, owner, (l3obj*)&classname,t_syv,retown,ifp);

     nobj->_type = t_syv;
     nobj->_parent_env  = env;

     *retval = nobj;

L3END(make_new_symvec)


L3METHOD(quote)

   l3path sexps(exp);

L3END(quote)


L3METHOD(test_symvec)

   make_new_symvec(L3STDARGS);
   l3obj* symvec = *retval;

   printf("should be empty symvec, just after creation:\n");
   symvec_print(symvec, "", 0);

//symvec_pushback(l3obj* symvec, l3path* addname, l3obj* addme);
//long symvec_size(l3obj* symvec);

   std::vector<char*> ac;
   ac.push_back((char*)"one");
   ac.push_back((char*)"two");
   ac.push_back((char*)"three");
   ac.push_back((char*)"four");
   ac.push_back((char*)"five");

   for (long i =0; i < 5; ++i) {

      l3path addname((char*)(ac[i]));
      l3obj* val_to_set = make_new_double_obj(i, owner, "val_to_set_test_symvec");
      symvec_set(symvec, i, addname(), val_to_set);
   }
   printf("after symvec_set(), symvec_print on symvec yields:\n");

   symvec_print(symvec, "", 0);



   l3obj* ret = 0;
   l3path nameback;
   l3path nslashn;
   for (long i = 0; i < 5; ++i) {
      symvec_get_by_number(symvec,i, &nameback, &ret);
      printf("\n\ngot, for item %ld in symvec:  nameback='%s'   ptr=%p\n",i,nameback(),ret);
      nslashn.pushf("/%s",nameback());
   }
   printf("symvec_print on symvec yields:\n");
   symvec_print(symvec, "", 0);

   printf("for comparison, name/name :'%s'\n",nslashn());


   l3obj* ret2 = 0;
   for (long i =0; i < 5; ++i) {
      symvec_get_by_name(symvec, ac[i], &ret2);
      printf("querying with symvec_get_by_name on %s yieled:\n", ac[i]);
      print(ret2,"",0);
   }
//   printf("after symvec_set(), symvec_print on symvec yields:\n");



    // simple linear search
    l3obj* needle = ret2;
    bool found = symvec_simple_linear_search(symvec,needle);
    printf("found = %d\n", (int)found);

    symvec_clear(symvec);
    printf("After clearing:\n");
    symvec_print(symvec, "", 0);

L3END(test_symvec)



L3KMIN(symvec_pushback,3)
{

    // first arg: the symbol vector
    l3obj* symvec = 0;
    ptrvec_get(vv,0,&symvec);
    assert(symvec);
    if (symvec->_type != t_syv) {
        printf("error: in expression '%s': 1st arg to symvec_pushback must be a symvev (type t_syv). Instead we saw type '%s'.\n",
               sexps(),symvec->_type);
        generic_delete(vv, L3STDARGS_OBJONLY);
        l3throw(XABORT_TO_TOPLEVEL);
    }


    // second arg: name of symbol
    l3path addname;
    l3obj* obj_addname=0;
    ptrvec_get(vv,1,&obj_addname);

    // and sanity check the name.
    if (obj_addname->_type != t_str) {
        printf("error: in expression '%s': 2nd arg to symvec_pushback was not type t_str (a string with the name of the symbol to create); instead we saw type '%s'.\n",
               sexps(),
               obj_addname->_type);
        generic_delete(vv, L3STDARGS_OBJONLY);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    string_get(obj_addname,0,&addname);
    long add_nmlen = addname.len();
    if (add_nmlen < 1) {
        printf("error: in expression '%s': 2nd arg to symvec_pushback was an empty string. Cowardly refusing to create unnamed symbol.\n",
               sexps());
        generic_delete(vv, L3STDARGS_OBJONLY);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    if (add_nmlen >= SYMBOL_NAME_LEN) {
        printf("error: in expression '%s': 2nd arg to symvec_pushback: symbol name too long: '%s' "
               "had length %ld. Maximum string size is %ld characters.\n", 
               sexps(),
               addname(),
               add_nmlen,
               (SYMBOL_NAME_LEN-1));
        generic_delete(vv, L3STDARGS_OBJONLY);
        l3throw(XABORT_TO_TOPLEVEL);
    }


    // third arg: object to point to.
    l3obj* addme=0;
    ptrvec_get(vv,2,&addme);
    assert(addme);
    LIVEO(addme);

    internal_symvec_pushback(symvec, addname(), addme);

}
L3END(symvec_pushback)



//void symvec_pushback(l3obj* symvec, l3path* addname, l3obj* addme) {
void internal_symvec_pushback(l3obj* symvec, const qqchar& addname, l3obj* addme) {
     assert(symvec);
     assert(addname.len());
     //assert(addname->len()>0);
     size_t sl = addname.len();
     assert(sl>0);

     long N = symvec_size(symvec);
     symvec_set(symvec,N,addname,addme);
     assert((N+1)==symvec_size(symvec));
}


long symvec_size(l3obj* symvec) {
     assert(symvec);

     Word_t    array_size;
     JLC(array_size, symvec->_judyL, 0, -1);

     return (long)array_size;
}


// fills in name as a side effect
symbol* symvec_get_by_number(l3obj* symvec, long i, l3path* name, l3obj** ret) {
     assert(symvec);
     assert(i>=0);
     symbol* psym = 0;

     PWord_t   PValue = 0;
     JLG(PValue,symvec->_judyL,i);
     if (!PValue) {
         printf("error in symvec_get_by_number: bad index request to symbol vector: index %ld not present.\n",i);
         l3throw(XABORT_TO_TOPLEVEL);
     } else {
         psym = (*((symbol**)PValue));
         assert(psym);
         *ret =  psym->_obj;
         name->init(psym->_name);
     }

     return psym;
}


symbol* symvec_get_by_name(l3obj* symvec, const qqchar& name, l3obj** ret) {
     assert(symvec);
     *ret = 0; // default value

     long sl = name.len();
     assert(sl >0);
     if (sl >= SYMBOL_NAME_LEN) {
         std::cout << "error in symvec_get_by_name: name symbol name too long: '"<< name << "' had ";
         printf("length %ld. Maximum string size is %ld characters.\n",sl,SYMBOL_NAME_LEN-1);
         l3throw(XABORT_TO_TOPLEVEL);
     }


    uint8_t      Index[BUFSIZ];            // string to sort.
    bzero(Index,BUFSIZ);
    name.copyto((char*)Index,BUFSIZ-1);

    PWord_t   PValue = 0;
    JSLG(PValue,symvec->_judyS,Index);
    if (!PValue) {
      return 0;

    } else {
      symbol* psym = (*((symbol**)PValue));
      assert(psym);
      *ret = psym->_obj;
      return psym;
    }

    return 0;
}


//
//
// _judyL[i]  -   >  symbol*  ->  l3obj*  and _name
// _judyS[_name] ->  symbol* (same one as above)
//
// return the address of the newly malloc-ed symbol struct that was set;
// 
// will replace any old of the same key.
//
symbol* symvec_set(l3obj* symvec, long i, const qqchar& addname, l3obj*  val_to_set) {
     assert(symvec);
     assert(i>=0);
     long sl = addname.len();

     assert(sl >0);
     if (sl >= SYMBOL_NAME_LEN) {
         std::cout << "error in symvec_set: addname symbol name too long: '"<< addname << "' had ";
         printf("length %ld. Maximum string size is %ld characters.\n",sl,(SYMBOL_NAME_LEN-1));
         l3throw(XABORT_TO_TOPLEVEL);
     }


    // check for existing, and delete if already present
    l3obj*  already = 0;
    symbol* psym2 = symvec_get_by_name(symvec, addname, &already);
    if (psym2) {
        if (already == val_to_set) {
            // replace self with self? a no-op. We are done.
            return psym2;
        }
        symvec_del(symvec, addname);
    }
    // INVAR: symvec does not contain psym->_name as a key

     // create the new symbol*
     symbol* psym = (symbol*)malloc(sizeof(symbol));
     symvec->_mytag->gen_push(psym); // does the free() for this symbol when Tag destructs.

     bzero(psym,sizeof(symbol));

     addname.copyto(psym->_name,SYMBOL_NAME_LEN-1);
     psym->_obj = val_to_set;


     // _judyL part: numerical index -> psym

     PWord_t   PValue = 0;
     JLI(PValue, symvec->_judyL, i);
     symbol** po = (((symbol**)PValue));
     *po = psym;

    // _judyS part : char[] -> psym

    //PWord_t   PValue2 = 0;
     //    JSLI(PValue2, symvec->_judyS, (uint8_t*)(psym->_name));
     //    po = (((symbol**)PValue2));
     //    *po = psym;

     llref* storeme = llref_new(psym->_name, val_to_set, 0,0, symvec->_mytag, 0);
     symvec->_judyS->insertkey((char*)psym->_name, storeme);

    return psym;
}


// symvec_del one entry
//
// _judyL[i]  -   >  symbol*
// _judyS[_name] ->  symbol*

void symvec_del(l3obj* symvec, const qqchar& delname) {

     assert(symvec);
     long sl = delname.len();
     assert(sl >0);
     if (sl >= SYMBOL_NAME_LEN) {
         std::cout << "error in symvec_del: delname symbol name too long: '"<< delname << "' had ";
         printf("length %ld. Maximum string size is %ld characters.\n",sl,(SYMBOL_NAME_LEN-1));
         l3throw(XABORT_TO_TOPLEVEL);
     }

     int Rc_word = 0;
     l3obj* ret = 0;
     symbol* psym = symvec_get_by_name(symvec, delname, &ret);
     if (0==psym) {
         std::cout << "error in symvec_del: symbol name '"<< delname << "' could not be found.\n";
         l3throw(XABORT_TO_TOPLEVEL);
     }
     
     if (symvec->_judyL) {
         JLD(Rc_word, symvec->_judyL, psym->_judyLIndex);
     }
     
     if (symvec->_judyS) {
         //JSLD(Rc_word, symvec->_judyS, (uint8_t*)psym->_name); 
         symvec->_judyS->deletekey(psym->_name);
     }

     symvec->_mytag->gen_remove(psym);
     free(psym);
}
// end symvec_del



void symvec_print(l3obj* symvec, const char* indent, stopset* stoppers) {
     assert(symvec);
  
     long N = symvec_size(symvec);
  
     printf("%s%p : (symvec of size %ld): \n", indent, symvec, N);
  
     l3path more_indent(indent);
     more_indent.pushf("%s","     ");

     // a second way, to confirm:
       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
      
       JLF(PValue, symvec->_judyL, Index);
       while (PValue != NULL)
       {
           //           printf("%ssymvec content [%02lu] =  '%p' ", indent, Index, (l3obj*)(*PValue));
           printf("%ssymvec[%02lu]=", indent, Index); //, (l3obj*)(*PValue));

           symbol* psym = (*((symbol**)PValue));
           assert(psym);
           l3obj* ptr = psym->_obj;

            printf("symbol{ %s } -> %p : ser# %ld  type %s ",psym->_name,ptr,ptr->_ser, ptr->_type);
            if (stoppers && obj_in_stopset(stoppers,ptr)) {
                 printf("(stopper)\n");
            } else {
                 print(ptr,more_indent(),stoppers);
            }
            JLN(PValue, symvec->_judyL, Index);
       }

}

void symvec_clear(l3obj* symvec) {
  assert(symvec);
  assert(symvec->_type == t_syv);

   long  Rc = 0;

   if (symvec->_judyL) {
       JLFA(Rc,  (symvec->_judyL));
   }

   if (symvec->_judyS) {
       //JLFA(Rc,  (symvec->_judyS));
       symvec->_judyS->clear();
   }

   assert(symvec_size(symvec)==0);
}


// simple linear search
bool symvec_simple_linear_search(l3obj* symvec, l3obj* needle) {

       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
      
       JLF(PValue, symvec->_judyL, Index);
       while (PValue != NULL)
       {
           symbol* psym = (*((symbol**)PValue));
           assert(psym);
           l3obj* ptr = psym->_obj;
           assert(ptr);
           if (ptr == needle) return true;
           JLN(PValue, symvec->_judyL, Index);
       }

  return false;
}

void test_symvec() {



}

L3METHOD(symvec_aref) 
{
   arity = 2; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   l3obj* symvec = 0;
   l3obj* whichindex = 0;
   ptrvec_get(vv, 0, &symvec);
   ptrvec_get(vv, 1, &whichindex);
   double dindex = -1;
   long   lindex = -1;
   l3path symbolname;

   XTRY
      case XCODE:

      if (whichindex->_type != t_dou && whichindex->_type != t_str) {
          printf("error in symvec_aref: index was not numeric nor was it string.\n");
          l3throw(XABORT_TO_TOPLEVEL);
      }

      if (whichindex->_type == t_dou) {
          dindex = double_get(whichindex,0);
          if (isnan(dindex) || isinf(dindex)) {
              printf("error in symvec_aref: bad index value: %f\n",dindex);
              l3throw(XABORT_TO_TOPLEVEL);
          }

          lindex = (long)dindex;
          symvec_get_by_number(symvec, lindex, &symbolname, retval);
          break;
      }
  
      if (whichindex->_type == t_str) {
          string_get(whichindex,0,&symbolname);
          if (!symvec_get_by_name(symvec, symbolname(), retval)) {
              printf("error in symvec_afref: bad index request to symbol vector: symbolname '%s' not present.\n",symbolname());
              l3throw(XABORT_TO_TOPLEVEL);
          }
          break;
      }

      break;

      case XFINALLY:
          generic_delete(vv, L3STDARGS_OBJONLY);
          break;
   
   XENDX



}
L3END(symvec_aref)


// (symvec_setf  symvec  symbolname  symbolvalue)
//
L3METHOD(symvec_setf)
{
   arity = 3; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   l3obj* symvec = 0;
   l3obj* snam = 0;
   l3obj* sval = 0;
   l3path nam;
   long sz = 0;

   XTRY
       case XCODE:

           ptrvec_get(vv, 0, &symvec);
           ptrvec_get(vv, 1, &snam);
           ptrvec_get(vv, 2, &sval);

           // take care of ownership problem... but what if it is already owned and permanant???
           sval->_owner->generic_release_to(sval, symvec->_mytag);
           
           if (symvec->_type != t_syv) {
               printf("error in symvec_setf: array was not a symbol vector (t_syv) but rather unexpected type %s.\n",symvec->_type);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           
           sz = symvec_size(symvec);
           string_get(snam, 0,&nam);
           symvec_set(symvec, sz, nam(), sval);
           
           *retval = sval;
           
       break;
       case XFINALLY:
           generic_delete(vv, L3STDARGS_OBJONLY);
       break;
   XENDX
}
L3END(symvec_setf)


L3METHOD(symvec_clear)
   symvec_clear(obj);
L3END(symvec_clear)


ostream& operator<<(ostream& os, const symbol& s) {
    os << "symbol { _name='" << s._name << "';   _obj=" << (void*)(s._obj)  << "  _judyLIndex=" << s._judyLIndex << "  }\n";
    return os;
}
