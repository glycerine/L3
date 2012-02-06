//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
 #define _ISOC99_SOURCE 1 // for NAN
 #include <math.h>
 #include <stdio.h>
 #include <limits.h>
 #include <climits>
 #include <time.h>
 #include <zmq.hpp>
 #include "autotag.h"
 #include "l3obj.h"
 #include "xcep.h"
 #include "merlin.pb.h"
 #include "qexp.h"
// #include <ffi.h>
 #include <cxxabi.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <string.h>
 #include <string>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include "rmkdir.h"
 #include <libgen.h>
 #include <assert.h>
 using std::string;



 #include "objects.h"

 #include <openssl/sha.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>
 #include <histedit.h>
 #include <termios.h>

// (c-set-style "whitesmith")

#ifndef _MACOSX

 #define HAVE_DECL_BASENAME 1
 #include <demangle.h>

#endif


 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler
 // quick type system; moved to quicktype.h/cpp
 #include "quicktype.h"

#include "serialfac.h"
#include "terp.h" // for eval()
#include "judydup.h"
#include "tostring.h"
#include "ut.h"

/////////////////////// end of includes



unittest::unittest() {
    max_test(); // initialize as side effect.
}

long unittest::last_test() { return _last_test; }

long unittest::max_test() {
    l3path tmp;
    pick_test(0,&tmp); // sets _max_test as side effect
    return _max_test;
}

long  unittest::run_next_test() {
  long lt = last_test();
  long mt = max_test();
  
  if (lt < mt) {
    unittest_cur = lt+1;
    unittest_max = unittest_cur;
    printf(" ;; (ut %ld %ld) requested by run_next_test().\n",unittest_cur, unittest_max);
  }
  return 0;
}

void unittest::pick_test(long testnum, l3path* lineout) {

    _last_test = testnum;

    switch(testnum) {
    case 1:
      lineout->init("  src (\"newpassing.l3\")");
      break;        
    case 2: 
      lineout->init("  b=(new b b)");
      break;
    case 3:
      lineout->init("  (assert (aref (exists \"b\") 0))");
      break;
    case 4:
      lineout->init("  (rm b)");
      break;
    case 5:
      lineout->init("  (assert (not (aref (exists \"b\") 0)))");
      break;

      // we should see a1 increment...
    case 6:
        //      lineout->init("  (:a1=5 )(de inca (prop) (= a1 (+ a1 1))) (inca)");
      break;

    case 7: {
        lineout->init("  (de add4 (prop (return res @< t_dou ) (arg x !> t_dou)) (+ x 4))");
    }
        break;
    case 8: {
        lineout->init("  (add4 5)");
    }
        break;
    case 9: {
        lineout->init("  (assert (eq 9 (add4 5)))");
    }
        break;

    case 10: {
        lineout->init("  (rm add4) (rm inca) (rm a1)");
    }
        break;
    case 11: {
        lineout->init("  (assert (allbuiltin))"); // check for memory leaks
    }
        break;

      case 12:
        lineout->init("  quiet");
        break;
      case 13:
        lineout->init("  (de sq (prop (return res @< t_dou) (arg x !> t_dou)) (* x x) )");
        break;
      case 14:
        lineout->init("  (de sum (prop (return res @< t_dou) (arg x !> t_dou) (arg y !> t_dou)) (+ x y) )");
        break;
      case 15:
       lineout->init("  (de sumsq (prop (return res @< t_dou) (arg x !> t_dou) (arg y !> t_dou)) (sum (sq x) (sq y)) )");
       break;
      case 16:
       lineout->init("  (assert (eq 25 (sumsq 3 4)))");
       break;
      case 17:
        lineout->init( "  (de plus12 (prop (return res @< t_dou) (arg x !> t_dou) )   (progn (setq a 10) (setq b 2) (+ a b x)))");
        break;
      case 18:
        lineout->init( "  (assert (eq 17 (plus12 5)))");
        break;

        // rm and cleanup
      case 19:
        lineout->init( "  (progn (rm sum) (rm sq) (rm sumsq) (rm plus12))");
        break;


        // someday... :-)
        //        lineout->init( "  a=b[ a > 3 && a < 2]");

        // dynamic var / llref->_obj update problem
      case 20: lineout->init( "  a=9   ");  break;
      case 21: lineout->init( "  b=2  ");  break;
      case 22: lineout->init( "  a=b   ");  break; // the a llref-ring now has _obj that points to 2. 9 should be deleted now, for lack of references.
      case 23: lineout->init( "  (softrm a) ");  break; //  b and 2 should still be alive. (rm a) would delete a and the b ref would die with it.
      case 24: lineout->init( "  (assert (aref (exists \"b\") 0))"); break;
      case 25: lineout->init( "  (rm b)"); break;
      case 26: lineout->init( "  (assert (not (aref (exists \"b\") 0)))"); break;

          // old way: a=a2 would just delete a first. That works. But. How do we @< pass-by-reference?
          
      case 30:
        lineout->init("  (assert (allbuiltin))"); // check for memory leaks
        break;

      case 31:
        lineout->init("  a = (new truck mytruck)");
       break;
      case 32:
        lineout->init("  (cd a)");
       break;
      case 33:      
        lineout->init("  (de drive2 (prop (return res @< t_dou) (arg x !> t_dou) )   (progn (setq b 2) (+ b x)))");
       break;
      case 34:
        lineout->init("  (assert (eq 12 (drive2 10)))");
        break;
      case 35:
        lineout->init("  ..");
        break;
      case 36:
        lineout->init("  (assert (eq 12 (a.drive2 10)))");
        break;
      case 37:
        lineout->init("  (rm a)");
        break;
      case 38:
        lineout->init("  (assert (allbuiltin))"); // check for memory leaks
        break;

      case 39:
        lineout->init("  (de plus12 (prop (return res @< t_dou) (arg x !> t_dou)) (progn (setq a 10) (setq b 2) (+ a b x)))");
        break;
      case 40:
          lineout->init("  (progn (:a2=(new a2 a2)) (:a = a2))"); // make an alias a, but a2 and a are immediately deleted...do we crash?
        break;
      case 41:
          lineout->init("  (setq a -5)");
        break;
      case 42:
          lineout->init("  (setq b -542)");
        break;

      case 43:
          lineout->init("  (assert (eq 62 (plus12  50)))"); // the external a and b should have no effect on the internal to plus12 a and b
        break;

      case 44:
          lineout->init("  a2 = a"); // make an alias a2 pointing to a
        break;

      case 45:
          //          lineout->init("  (progn (:a = -8) (assert (eq a2 a)))");
          lineout->init("  (progn (:a = -8) (assert (not (eq a2 a))))"); // we may want them eq instead of not.
        break;

        // the progn above was deleting a rather than updating it! Test that a still lives.
      case 46:
          lineout->init("  (assert (aref (exists \"a\") 0))");
       break;

      case 47:
        lineout->init("  (progn (rm a) (rm b) (rm a2) (rm plus12))"); // a2 should disappear with a. <-hmm not any more!
        break;
      case 48:
        lineout->init("  (assert (allbuiltin))"); // check for memory leaks
        break;

        // transfer owner ship into an object.
      case 50:
        lineout->init("  (poptop)"); // goto toplevel
        break;

      case 51:
        lineout->init("  tr = (new truck mytruck)");
        break;

      case 52:
        lineout->init("  tr .");
        break;
      case 53:
        lineout->init("  engine = (new v6 ed_the_engine)");
        break;

      case 54:
        lineout->init("  ..");
        break;

      case 55:
        lineout->init("  tr.engine.piston = 6.5");
        break;

      case 56:
        lineout->init("  tr.engine = 6");
        break;

        // don't split on dots at first.
#if 0  // eval_dot is not working, and its not a priority.
      case 57:
        lineout->init("  (. tr (= wheels 4))");
        break;
#endif
      case 58:
        lineout->init("  tr.wheels = 4");
        break;

      case 59:
        lineout->init("  (tr.wheels) = 4");
        break;

      case 60:
        lineout->init("  (de tr.takeit (prop (arg serverownsit @> t_obj )) 42)");
        break;
      case 61:
          //        lineout->init("  (de tr.takeit (prop (arg  serverownsit @> t_obj )) 42)");
        break;
      case 62:
          //        lineout->init("  (de tr.takeit (prop (arg  serverownsit @> t_any )) 42)");
        break;
      case 63:
        lineout->init("  (tr.takeit 87)");
        break;

        // test recursive action of rm through deleting tr a multilevel object
      case 68:
        lineout->init( "  (progn (poptop) (rm tr))");
        break;

      case 69:
        lineout->init("  (assert (allbuiltin))"); // check for memory leaks
        break;



        // tests for recursive.dot.notation on the left hand side. Or rhs too.
      case 70:
        lineout->init("  ca = (new car mycar)");
        break;
      case 71:
        lineout->init("  ca .");
        break;
      case 72:
        lineout->init("  trunk = (new trunkobj mytr)");
        break;
      case 73:
        lineout->init("  ..");
        break;
      case 74:
        lineout->init("  ca.trunk.kit = 5");
        break;
      case 75:
        lineout->init("  ca2=(cp ca)");
        break;
      case 76:
        lineout->init("  ca2.trunk .");
        break;

      case 77:
        lineout->init("  (rm ca2)"); // this should fail to remove ca2 because we are inside it.
        break;
      case 78:
        lineout->init("  (assert (aref (exists \"ca2\") 0))"); // confirm still around
        break;
      case 79:
        lineout->init("  (progn (rm ca2))"); // this should fail to remove ca2 because we are inside it, too.
        break;
      case 80:
        lineout->init("  (assert (aref (exists \"ca2\") 0))"); // confirm still around
        break;

        // actually clean up
      case 81:
        lineout->init("  poptop");
        break;
      case 82:
        lineout->init("  (progn (rm ca2) (rm ca))");
        break;
      case 83:
        lineout->init("  (assert (allbuiltin))");
        break;


        // tests for server objects and clients ending up with ownership after @> and @< calls.
        
      case 84:
        lineout->init("  sylv=(new sylvester sylvester)");
        break;
      case 85:
        lineout->init("  tracerx = (new tracer tracerx)");
        break;
      case 86:
        lineout->init("  (de sylv.takeit (prop (arg  serverownsit @> t_obj )) 42)"); // only takes objects
        break;
      case 87:
        lineout->init("  (de sylv.takeit (prop (arg  serverownsit @> t_any )) 42)"); // t_any means it should take a double.
        break;
      case 88:
        lineout->init("  (sylv.takeit 87)");
        break;
      case 89:
        lineout->init("  (sylv.takeit tracerx)");
        break;

      case 90:
        lineout->init("  (rm sylv)");
        break;
      case 91:
          //        lineout->init("  ");
        break;
      case 92:
        lineout->init("  (assert (allbuiltin))");
        break;

        // string handling / printf
      case 93:
        lineout->init("  a=(p \"once \" \"upon \" \"a time...\")");
        break;
      case 94:
        lineout->init("  b=(p)");
        break;
      case 95:
        lineout->init("  (de prefix (prop) \"mystring is \")");
        break;
      case 96:
        lineout->init("  e=(p (prefix) a)");
        break;
      case 97:
        lineout->init("  missing=(p \"heya\" (prefix))");
        break;

    // stdout, stderr, stdin
      case 98:
        lineout->init("  (so \"\nthis goes to stdout.\n\")");
      break;
      case 99:
        lineout->init("  (se \"\nthis goes to stderr.\n\")");
      break;
      case 100:
        lineout->init("  oneline=(si)"); // read one line from stdin
      break;


    // cond statements / bool expressions
      case 101:
        lineout->init("  (if 0 (so \"\nI see true!!\n\") (so \"\nI see false!\n\"))");
        break;
      case 102:
        lineout->init("  (if 0 (so \"\nI see true!!\n\"))");
        break;
      case 103:
        lineout->init("  (if 1 (so \"\nI see true!!\n\"))");
        break;
      case 104:
        lineout->init("  (if 1 (so \"true\") (so \"false\"))");
        break;
      case 105:
        lineout->init("  f = (if 0 (so \"I see true!!\") (so \"I see false!\"))");
        break;
      case 106:
        lineout->init("  t = (if 1 (so \"I see true!!\") (so \"I see false!\"))");
        break;
      case 107:
        lineout->init("  t = (if (+ 1 -1) (so \"I see true!!\") (so \"I see false!\"))");
        break;

        // cleanup cond, strings and so
      case 108:
          lineout->init("  (progn (rm e) (rm t) (rm f) (rm a) (rm b) (rm prefix) (rm missing))");
        break;
      case 109:
          lineout->init("  (assert (allbuiltin))"); // separate line to avoid having to mark additional captags from progn as builtin. (which would hide them from leak detection, which would be bad).
        break;

        // factorial
      case 110:
        lineout->init("  (de fact (prop (return x !< t_dou ) (arg y !> t_dou)) (if (eq y 1) 1 (* y (fact (- y 1)))))");
        break;
      case 111:
        lineout->init("  f6 = (fact 6)");
        break;
      case 112:
        lineout->init("  (assert (eq 720 f6))");
        break;
      case 113:
        lineout->init("  (progn (rm f6) (rm fact))");
        break;
      case 114:
        lineout->init("  (assert (allbuiltin))"); // cleanup, and check for leakage.
        break;

        //  @< creation and transfer out (factory method) using arg (vs using return).
        //  needs to work for testing add_dtor ...
      case 115:
        lineout->init("  myfx = -2");
        break;
      case 116:
        lineout->init("  (de upperputter (prop (arg sidefx @< t_any )) (progn (so \"upperputter running, with out arg sidefx getting 57.\") (:sidefx = 57) ))");
        break;
      case 117:
        lineout->init("  (upperputter myfx)");
        break;
      case 118:
          lineout->init("  (assert (aref (exists \"myfx\") 0))");
        break;
      case 119:
          lineout->init("  (assert (eq 57 myfx))"); // out arg worked
        break;
      case 120:
          lineout->init("  (progn (rm upperputter) (rm myfx))"); // cleanup
        break;
      case 121:
        lineout->init("  (assert (allbuiltin))"); // cleanup, and check for leakage.
        break;



        // add_dtor
      case 122:
        lineout->init("  (:sentinel = 0)  (:o = (new truck mytruck))");
        break;
      case 123:
          lineout->init("  (assert (aref (exists \"sentinel\") 0))");
        break;
      case 124:
        lineout->init("  (progn (de mydtor (prop) (progn (so \"mydtor for truck running.\") (= sentinel (+ 1 sentinel))))  (dtor_add mydtor o) )");
        break;

        // dtor firing...
      case 125:
        lineout->init("  o2=(cp o)");
        break;
      case 126:
        lineout->init("  (rm o)");
        break;
      case 127:
        lineout->init("  (progn (assert  (aref (exists \"sentinel\") 0) ) (assert (eq 1 sentinel))   (assert  (aref (exists \"sentinel\") 0) ))"); // dtor running was deleting the sentinel variable!?!?
        break;
      case 128:
        lineout->init("  (rm o2)");
        break;
      case 129:
        lineout->init("  (assert (eq 2 sentinel))");
        break;
      case 130:
        lineout->init("  (rm sentinel) (rm mydtor)");
        break;
      case 131:
        lineout->init("  (assert (allbuiltin))"); // cleanup, and check for leakage.
        break;


    // cp - deep copy of objects
      case 132:
        lineout->init("  t1 = (new truck mytr1_t1)");
        break;
      case 133:
        lineout->init("  t1.ff = 55");
        break;
      case 134:
        lineout->init("  (de truckdtor (prop) (so \"*** --->  truckdtor! <--- ***\"))");
        break;
      case 135:
        lineout->init("  (de t1.doit (prop) (so \"*** --->  doit running!!!!! <--- ***\"))");
        break;

      case 136:
        lineout->init("  (dtor_add truckdtor t1)");
        break;

      case 137:
        lineout->init("  t2 = (cp t1)");
        break;

      case 138:
        lineout->init("  (rm t1)");
        break;

      case 139:
        lineout->init("  t2.ff");
        break;
      case 140:
        lineout->init("  (t2.doit)");
        break;

        // @> methods to effect transfer, owns, ownlist, in
      case 141:
        lineout->init("  chest = (new pirate_chest  chest_wants_booty)");
        break;
      case 142:
          //        lineout->init("  (de takeit (prop (arg takem @> t_obj )) (so \"takeit running!\" ))");
        break;
      case 143:
          //        lineout->init("  (transfer takeit chest)"); // first we have to assign ownership of the method to the object
                                                     //   and then the method can know who to transfer ownership to(!)
        break;
      case 144:
        lineout->init("  booty = (new  booty  mybooty_to_transfer_in)");
        break;
      case 145:
          //       lineout->init("  (chest.takeit booty)");
       lineout->init("  (transfer booty chest)");
        break;
      case 146:
          //        lineout->init("  (assert (owns booty chest))"); // owns return T or F, assert, if false, assert(0)s.
          lineout->init("  ol = (ownlist chest)");
        break;
      case 147:
          lineout->init("  (in booty ol)"); // (ownlist ...) returns a ptrvec of objects owned by t2
                                                             // (in a b) returns T if object a is in the ptrvec b
        break;
      case 148:
          //        lineout->init("  ol = (ownlist chest)");
        break;
      case 149:
          //        lineout->init("  (assert (exists ol))");
        break;


      case 151:
        lineout->init("  o=(new oo ooo)");
        break;
      case 152:
        lineout->init("  (ownlist o)");
        break;

      case 161:
        lineout->init("  (progn (= a (v)) (assert( not (in a 3))))");
        break;
      case 162:
        lineout->init("  (progn (= a (v)) (in a 3) (assert (eq 0 (len a))))");
        break;
      case 163:
        lineout->init("  (assert (not (in 2 nan)))");
        break;

        // transfer
      case 171:
        lineout->init("  o=(new oo ooo)");
        break;

      case 172:
        lineout->init("  truck = (new truck mytruck)");
        break;
        
      case 173:
        lineout->init("  (transfer o truck)");
        break;
        
      case 174:
        lineout->init("  (so \"This shold be true--> \" (owns truck.o truck)))");
        break;

    // exists, lookup
      case 181:
          lineout->init("  o = (new o o)");
          break;
      case 182:
          lineout->init("  (if (exists \"o\") (so \"yes to o!\") (so \"no to o!\"))");
          break;
      case 183:
          lineout->init("  (if (lookup \"o\") (so \"yes to o!\") (so \"no to o!\"))");
          break;
      case 184:
          lineout->init("  (assert (exists \"o\"))");
          break;          
      case 185:
          lineout->init("  (assert (lookup \"o\"))");
          break;          

          // ioprim
      case 191:
          lineout->init("  f=(fopen \"/tmp/testfile\" \"w\")");
          break;          
      case 192:
        lineout->init("  (fprint f \"a message out\n\")");
          break;          
      case 193:
        lineout->init("  (rm f)");
          break;          

      case 194:
          lineout->init("  f2=(fopen \"/tmp/testfile2\" \"w\")");
          break;          
      case 195:
        lineout->init("  (fprint f2 \"a message out\n\")");
          break;          
      case 196:
        lineout->init("  (fflush f2)");
          break;          
      case 197:
        lineout->init("  (fprint f2 \"more message after flush!\nAND a little more\n\")");
          break;          
      case 198:
        lineout->init("  (fclose f2)");
          break;          


      case 201:
          lineout->init("  f3=(fopen \"/tmp/testfile2\" \"r\")");
          break;          
      case 202:
        lineout->init("  s1=(fgets f3)");
          break;          
      case 203:
        lineout->init("  s2=(fgets f3)");
          break;          
      case 204:
        lineout->init("  (assert (eq s1 \"a message out\n\"))");
        break;
      case 205:
        lineout->init("  (assert (eq s2 \"more message after flush!\n\"))");
          break;          
      case 206:
        lineout->init("  c1=(fgetc f3)");
          break;          
      case 207:
        lineout->init("  (assert (eq c1 \"A\"))");
          break;
      case 208:
        lineout->init("  (assert (eq \"ND\" (nexttoken f3)))");
          break;
      case 209:
        lineout->init("  (fclose f3)");
          break;
      case 210:
        lineout->init("  (rm f3)");
          break;

          // nexttoken tests
      case 211:
          lineout->init("  f=(fopen \"/tmp/token_testfile\" \"w\")");
          break;          
      case 212:
          lineout->init("  (fprint f \"hi there;jason \\\"this is a string\\\"; set of \\\"tokens; tokens; tokens;\\\"\")");
          break;
      case 213:
          lineout->init("  (fclose f)");
          break;
 
      case 214:
          lineout->init("  f=(fopen \"/tmp/token_testfile\" \"r\")");
          break;          
      case 215:
          lineout->init("  (assert (eq  (nexttoken f) \"hi\"))");
          break;
      case 216:
          lineout->init("  (assert (eq  (nexttoken f) \"there\"))");
          break;
      case 217:
          lineout->init("  (assert (eq  (nexttoken f) \";\"))");
          break;
      case 218:
          lineout->init("  (assert (eq  (nexttoken f) \"jason\"))");
          break;
      case 219:
          lineout->init("  (assert (eq  (nexttoken f) \"\\\"this is a string\\\"\"))");
          break;
      case 220:
          lineout->init("  (assert (eq  (nexttoken f) \";\"))");
          break;

      case 221:
        lineout->init("  (assert (eq  (nexttoken f) \"set\"))");
          break;
      case 222:
          lineout->init("  (assert (eq  (nexttoken f) \"of\"))");
          break;
      case 223:
          lineout->init("  (assert (eq  (nexttoken f) \"\\\"tokens; tokens; tokens;\\\"\"))");
          break;
      case 224:
          lineout->init("  (rm f)");
          break;
      case 225:
          lineout->init("  (de manufacture (prop (return var @< t_any)) (new o o))");
          break;

          // foreach loops
      case 228:
          lineout->init("  coll=(v 4 6 8)");
          break;
      case 229:
          lineout->init("  (foreach coll i c (so \"the \" i \"-th item is: \" c \", a.k.a. :\" (aref coll i)))");
          break;
      case 230:
          lineout->init("  cols=(v \"a one\"  \"a two\" \"a three\")");
          break;
      case 231:
          lineout->init("  (foreach cols j s (assert (aref cols j) s))");
          break;

          // while1 loops
      case 234:
          lineout->init("  (progn (:i=0) (:i = (+ i 1)) (if (> i 4) (break_) (so \"i is \" i) ))");
          break;
      case 235:
          lineout->init("  (progn (:i=0) (while1 (progn (:i = (+ i 1)) (if (> i 4) (break_) (so \"i is \" i) ))))");
          break;
      case 236:
          lineout->init("  (try (throw \"'this is being thrown'\") (catch (so \"in catch\")) (finally (so \"in finally\")))");
          break;
      case 237:
          lineout->init("  cols=(v \"a one\"  \"a two\" \"a three\")");
          break;
      case 238:
          //          lineout->init("  (foreach cols j s (assert (eq (aref cols j) s)))");
          lineout->init("  (foreach cols j s (so \"j is \" j \" and s is '\" s \"'\"))");
          break;

      case 239:
          lineout->init("  (foreach cols j s (so \"s is '\" s \"', and aref of j=\" j \" gives: '\" (aref cols j) \"'\"))");

          // problem: s is "oneshot"!! can't evaluate it more than once????
          // compare
          // (foreach cols j s (so "<- s; begin: " j (aref cols j) " and more with s being: " s))
          // vs
          // (foreach cols j s (so s "<- s; begin: " j (aref cols j) " and more with s being: " s))
          break;
          /*
          // recursive functions that return transfer ownership.
      case 241:
          lineout->init("  (de spawn (prop (return simon @< t_any ) ) (progn (new cupy cupster) (so \"just made a cupster!\")))");
          break;
      case 242:
          lineout->init("  (de tospawn (prop (return sim2 @< t_any ) ) (progn (setq myspawn (spawn)) (setq b (new doggy dogster)) (setq b.shrimp myspawn) b))");
          break;
      case 243:
          lineout->init("  sh=(tospawn)");
          break;
      case 244:
          lineout->init("  (assert (lexists \"sh\"))");
          break;

          // recursion and @< ownership semantics

      case 245:
          lineout->init("  (..)");
          break;
      case 246:
          lineout->init("  (..)");
          break;
          */
          /*
      case 247:
          lineout->init("  (if (exists \"ca\") (rm ca))");
          break;

      case 248:
          lineout->init("  (if (exists \"a\") (rm a))");
          break;
          */

      case 249:
          lineout->init("  (progn (:e1 = (new e1 e1)) (:e2 = (new e2 e2)) (:e3 = (new e3 e3)))");
          break;
      case 250:
          lineout->init("  (progn (de func1 (prop (return gold1 @< t_any)) ( : gold1 = (new g1 g1)))  (transfer func1 e1))");
          break;
      case 251:
          lineout->init("  (progn (de func2 (prop (return booty2 @< t_any)) (progn (e1.func1) ( : booty2 = (e1.func1))))  (transfer func2 e2))");
          break;
      case 252:
          lineout->init("  (progn  (de func3 (prop (return silver2 @< t_any)) (: silver2 = (e2.func2)))  (transfer func3 e3) )");
          break;
      case 253:
          lineout->init("  (e3.func3)");
          break;
      case 254:
          lineout->init("  (e2.func2)");
          break;
      case 255:
          lineout->init("  (e1.func1)");
          break;

      case 256:
          lineout->init(" \
(de recurs \
    (prop \
          (return res    @< t_obj ) \
          (arg how_deep  !> t_dou) \
          (arg wrapme    @> t_obj ))  \
    (if (eq 0 how_deep) \
        (:res = wrapme) \
      (progn \
         (: tmp = (new o o)) \
         (transfer wrapme tmp) \
         (: res = (recurs (- how_deep 1) tmp))))) \
");
          break;

      case 257:
          lineout->init("  starter=(new starter starter)");
          break;

      case 258:
          lineout->init("  a=(recurs 1 starter)");
          break;

      case 259:
          lineout->init("  b=(recurs 2 starter)"); // failing to test anyway: (assert (not (exists \"tmp\"))))");
          break;

      case 260:
          lineout->init("  fiver=(recurs 5 starter)"); // (assert (not (exists \"tmp\"))))");
          break;

          // pop_to_env
          // pop_to_tag
      case 271:
          lineout->init("  (de  f (prop) (progn (:o = (new o o)) (cd o) ))");
          break;

      case 272:
          lineout->init("  (poptop)");
          break;


      case 281:
          lineout->init("  (rm tr)");
          break;

      case 282:
          lineout->init("  (rm ca)");
          break;

      case 283:
          //lineout->init("  (rm a)");
          break;

      case 284:
          lineout->init("  lsb");
          break;



          // aliases and llref related functions
      case 291:
          lineout->init("  (poptop)");
          break;
      case 292:
          lineout->init("  (progn (:a=(new a a)) (:a.b=(new b b)) (:d = a.b))");
          break;
      case 293:
          lineout->init("  al=(aliases d)");
          break;
      case 294:
          lineout->init("  (eq 3 (len al))");
          break;

          //          cleanup
      case 298:
          lineout->init("  (poptop)");
          break;
      case 299:
        lineout->init("  (rm truck truckdtor t2 s1 s2 recurs ol o manufacture i f2 e3 e2 e1 d cols coll chest c1 al a f)"); // cleanup, and check for leakage.
        break;
      case 300:
        lineout->init("  (assert (allbuiltin))"); // cleanup, and check for leakage.
        break;

          // nca and setq automatic uptree promotion upon alias creation
      case 301:
          lineout->init("  (poptop)");
          break;
      case 302:
          lineout->init("  (progn (:A=(new A A)) (:A.b=(new b b)) (:d = A.b))");
          break;
      case 303:
          lineout->init("  ");
          break;
      case 304:
          lineout->init("  ");
          break;

          // symvec:

          // void symvec_pushback(l3obj* symvec, l3path* addname, l3obj* addme);
          // long symvec_size(l3obj* symvec);
          // void symvec_get_by_number(l3obj* symvec, long i, l3path* name, l3obj** ret);
          // void symvec_get_by_name(l3obj* symvec, char* name, l3obj** ret);
          // void symvec_set(l3obj* symvec, long i, l3path* addname, l3obj*  val_to_set);
          // void symvec_print(l3obj* symvec, const char* indent, stopset* stoppers);
          // void symvec_clear(l3obj* symvec);          
          // bool symvec_search(l3obj* symvec, l3obj* needle);

    case 310:
        lineout->init("  a=(make_new_symvec)");
        break;
    case 311:
        lineout->init("  (symvec_setf a \"howdy\" 88)");
        break;
    case 312:
        lineout->init("  (symvec_setf a \"howdy\" -45)");
        break;
    case 313:
        lineout->init("  (assert (eq (aref a 0) -45))");
        break;

    case 320:
        lineout->init("  (symvec_setf a \"george\" 75)");
        break;
    case 321:
        lineout->init("  (assert (eq (aref a 1) 75))");
        break;
    case 322:
        lineout->init("  (assert (eq (symvec_aref a \"howdy\") 88))");
        break;
    case 323:
        lineout->init("  (assert (eq (symvec_aref a \"george\") 75))");
        break;

    case 330:
        lineout->init("  sv=(make_new_symvec)");
        break;
    case 331:
        lineout->init("  (symvec_setf sv \"qui\" 3)");
        break;
    case 332:
        lineout->init("  (symvec_setf sv \"pingme\" 22)");
        break;
    case 333:
        lineout->init("  (symvec_setf sv \"pingme\" 11)");
        break;
    case 334:
        lineout->init("  (eq (symvec_aref sv \"pingme\") 11)");
        //        lineout->init("  (assert (eq (symvec_aref sv \"pingme\") 11))");
        break;


        // test that cycles get resolved okay
    case 340:
        lineout->init("  (progn (poptop) (:a=(new a a)) (:b=(new b b)) (:a.bb = b) (:b.aa = a) (rm a) (rm b)  )");
        break;


        // test that function can change vars in the env above, static scope:
    case 350:
        lineout->init("  bumpme=7");
        break;
    case 351:
        lineout->init("  (progn (de uppermoder (prop) (progn (= bumpme (+ 1 bumpme)))))");
        break;
    case 352:
        lineout->init("  (uppermoder)");
        break;
    case 354:
        lineout->init("  (assert (aref (exists \"bumpme\") 0))");
        break;
    case 356:
        lineout->init("  (assert (eq bumpme 8))");
        break;
    case 357:
        lineout->init("  (rm bumpme)");
        break;
    case 358:
        lineout->init("  (rm uppermoder)");
        break;
    case 359:
        lineout->init("  (assert (allbuiltin))"); // cleanup, and check for leakage.
        break;


        // test that *progn* can change vars in the env above, static scope:
    case 360:
        lineout->init("  bumpme=7");
        break;
    case 361:
        lineout->init("  (progn (= bumpme (+ 1 bumpme)))");
        break;
    case 362:
        lineout->init("  ");
        break;
    case 364:
        lineout->init("  (assert (aref (exists \"bumpme\") 0))");
        break;
    case 366:
        lineout->init("  (assert (eq bumpme 8))");
        break;
    case 367:
        lineout->init("  (rm bumpme)");
        break;
    case 368:
        //        lineout->init("  (rm uppermoder)");
        break;
    case 369:
        lineout->init("  (assert (allbuiltin))"); // cleanup, and check for leakage.
        break;

        
      }



    _max_test = 400;
}



/// cmdhistory
void cmdhistory::sync() { if (_histlog) { _histlog->jmsync(); } }


void cmdhistory::monitor_this_tag(Tag* pt) {
  _monitor_tags.push_back(pt);
}


void cmdhistory::init(l3path histpath) {

    _histlog = new jmemlogger;

    assert(_histlog);
    _histlog->init(histpath);

    _history_len = 0;
    _history_last = 0;
    _history_max = 1;
    _monitor_tag = 0;

    _monitor_tags.clear();

    init_editline_history();
}

cmdhistory::~cmdhistory() {
    
    if (_histlog) {
        delete _histlog;
    }
    _monitor_tags.clear();

    if (_history_last) {
        // is it alive? better not be.
        _history_last = 0;
    }

    assert(_history_list.size() == 0); // o/w we have to iterate and delete. partially done below.
    if (_history_list.size()) {
        _history_list.clear();
    }

    teardown_editline_history();
}


//
//
// 
void cmdhistory::last_cmd_had_value(l3path* cmd, l3obj** pvalue, Tag* owner) {
    FILE* ifp = 0;

    assert(pvalue);
    l3obj* value = *pvalue;

    // we allow 0x0 values now... and we just skip them.
    if (!value) return;
    LIVEO(value);

            if (refcount_minimal(value)) {

                generic_delete(value,-1,0,  main_env,0,owner,  0,0,owner,ifp);
                *pvalue = 0;
                value = 0;
            } else if (_monitor_tags.size()) {

            // old way should probably be enhanced to scan direct subtags of monitor_tags,
            //  so that captags can be detected.

            // old way:
            // how do we know if this value isn't in use by anyone but the toplevel env?
            //  main_env has a tag, and we should be able to ask if this item, in this tag,
            //  has a listing in the captain of the tag or not.


              list_itag_it be = _monitor_tags.begin();
              list_itag_it en = _monitor_tags.end();

              for (; be != en; ++be ) {
                  _monitor_tag = *be;

                  if (_monitor_tag->owns(value)) {
                      l3obj* cap = _monitor_tag->captain();

                      if (cap) {
                          if (!hashtable_has_pointer_to(cap,value)) {

                              DV(printf("*** deleting %p (ser# %ld) '%s' because "
                                        "it is owned by main_env but has no name "
                                        "there.\n",
                                        value,value->_ser, value->_varname));

                           generic_delete(value,  -1,0,  0,0,owner,  0,0,owner,ifp);
                           value = 0;

                          }
                      }
                  }
              }

            } // end if monitor_tags.size()
            
    
} // end last_cmd_had_value




void cmdhistory::show_history() {
    sync();
    char* path = _histlog->get_mypath();
    FILE* f = fopen(path,"r");
    if (0==f) {
        printf("error in cmdhistory::show_history() could not fopen this path '%s': %s\n", path, strerror(errno));
        l3throw(XABORT_TO_TOPLEVEL);
    }
    l3path line;
    long i = 0;
    while(1) {
        if (NULL == fgets(line(), line.bufsz(), f)) break;
	printf("history %03ld: ",i);
        line.out();
	++i;
    }
    if (f) fclose(f);
}



void cmdhistory::teardown() {
    if (_histlog) { 
        delete _histlog;
        _histlog=0;
    }

    if (_editline) {
        teardown_editline_history();
        _editline = 0;
        _editline_history = 0;
    }
}



void cmdhistory::add(const char* msg) {
    _histlog->add(msg);
}



void cmdhistory::add_sync(const char* msg) {
    _histlog->add_sync(msg);
}



long cmdhistory::keeping_last() { return _history_max; }

L3METHOD(history_limit_get)

   arity = num_children(exp);
   if (arity != 1) {
      printf("error: history_limit_get takes no arguments.\n");
      l3throw(XABORT_TO_TOPLEVEL);    
   }
   double dkeeplast = histlog->keeping_last();

   if ((*retval) && (*retval)->_type == t_dou) {
       double_set(*retval,0, dkeeplast);
       return 0;
   } else {
      *retval = make_new_double_obj(dkeeplast,retown,"history_limit_get");
   }

L3END(history_limit_get)


void cmdhistory::keep_last(long ncmds_as_history) {
   _history_max = ncmds_as_history;
}

L3METHOD(history_limit_set)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   l3obj* newlim = 0;
   double dval = 0;

   XTRY
       case XCODE:

           ptrvec_get(vv, 0, &newlim);
           if (newlim->_type != t_dou) {
               printf("error: history_limit_set needs a numeric argument--the count of history values to keep around.\n");
               l3throw(XABORT_TO_TOPLEVEL);
           }
           dval = double_get(newlim,0);
           histlog->keep_last((long)dval);

       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

L3END(history_limit_set)


// check while running and confirm there have been no memory leaks...this
// wants only is_sysbuiltin objects to remain, any others it will return 0;
// Returns 1 if only sysbuiltin objects are allocated.
L3METHOD(heap_is_sysbuiltin_only)

L3END(heap_is_sysbuiltin_only)


char* _get_prompt_for_editline(LibEditLine::EditLine* editline) {

       // global histlog reference
       return histlog->get_prompt();
}

   //
   // from readline.c's rl_initialize, how to start up the libedit editline History* using library.
   //
void cmdhistory::init_editline_history() {

    // editline library doesn't clean up it's internals well...so to prevent false positives in sermon,
    // we look aside as we init here:
#ifndef  _DMALLOC_OFF
       gsermon->off();
#endif
       _editline = LibEditLine::el_init("terp", stdin, stdout, stderr); // (EditLine *) 0x7f13d548a800
       _editline_history = LibEditLine::history_init(); // (History *) 0x7f13d54d9680

       // register the callback function that the library uses to obtain the prompt string
       LibEditLine::el_set(_editline, EL_PROMPT, _get_prompt_for_editline);

#ifndef  _DMALLOC_OFF
    gsermon->on();
#endif

}

void cmdhistory::teardown_editline_history() {

    if (_editline_history) {
        LibEditLine::history_end(_editline_history); // (History *) 0x7fb0da729680
    }
    _editline_history=0;

    if (_editline) {
        LibEditLine::el_end(_editline); // how we need to cleanup. // (EditLine *) 0x7fb0da6da800
    }
    _editline=0;
}


char* cmdhistory::readline(const char *prompt)
{
	int count;
	const char *ret;

    histlog->set_prompt((char*)prompt);

	ret = LibEditLine::el_gets(_editline, &count);

	if (ret && count > 0) {
		char *foo;
		int lastidx;

		foo = strdup(ret);
		lastidx = count - 1;
		if (foo[lastidx] == '\n')
			foo[lastidx] = '\0';

		ret = foo;
	} else
		ret = NULL;

	return (char *) ret;
}


void cmdhistory::add_editline_history(const char *line)
{
    LibEditLine::HistEvent ev;
    LibEditLine::history(_editline_history, &ev, H_ENTER, line);

}

// portions of the above readline() code are derived from
//  libedit/readline.c under the following license:

/*	$NetBSD: readline.c,v 1.19 2001/01/10 08:10:45 jdolecek Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or proomte products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

