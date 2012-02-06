#include <stdio.h>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <climits>

#include "l3path.h"
#include "rmkdir.h"
#include "unistd.h"
#include "dv.h"
#include "quicktype.h"
#include "lex_twopointer.h"

#include "l3ts.pb.h"
#include "l3path.h"
#include "l3ts_common.h"
#include "rmkdir.h"
#include "l3obj.h"
#include "autotag.h"
#include "terp.h"
#include "objects.h"
#include "l3string.h"
#include <list>
#include "l3pratt.h"
#include "l3ts.pb.h"
#include "l3ts_server.h"
#include "quicktype.h"

using std::vector;

#ifndef _MACOSX
int isnumber(int c);
#endif


//
// put the various munch_left and munch_right functions here in their own file.
//

qtree* infix_munch_left(qtree* _this_, qtree* left) {
    _this_->add_child(left);
    _this_->add_child( _this_->_qfac->expression(_this_->_lbp));
    return _this_;
}


qtree* infix_shortckt_munch_left(qtree* _this_, qtree* left) {
    _this_->add_child(left);
    _this_->add_child( _this_->_qfac->expression(_this_->_lbp - 1)); // the -1, reduced binding power on the second term, is what makes it short circuit...apparently.
    return _this_;
}


qtree* prefix_munch_right(qtree* _this_) {
    _this_->add_child( _this_->_qfac->expression(_this_->_rbp));
    return _this_;
}



qtree* group_munch_right(qtree* _this_) {
         assert(_this_->_closer);

         long limit = 10000;
         long i = 0;

         qtree* e = 0;
         t_typ  nexty = 0;
         while (1) {

             // check to see if we have termination next.
             nexty = _this_->_qfac->token()->_ty;
             if (nexty == _this_->_closer) {
                 _this_->_qfac->advance(_this_->_closer);
                 return _this_;
             }
             if (nexty == t_eof) {
                 return _this_;
             }

             // comma? skip it, not giving m_mns::left_munch a chance to run.
             if (nexty == t_comma) {
                 _this_->_qfac->advance(t_comma);
                 continue;
             }

             e = _this_->_qfac->expression(0);

             if (!e) {
                 return _this_;
             }
             if (e->_ty == _this_->_closer) {
                 return _this_;
             }
             if (e->_ty == t_eof) {
                 return _this_;
             }

             assert(e != _this_);
             _this_->add_child(e);

             if (++i == limit) {
                 printf("error in  qtree_%s::munch_right(): reached inf "
                        "loop detection of %ld iterations inside a '(': .\n",_this_->_ty, limit);
                 _this_->_qfac->print_error_context(_this_->_id);
                 l3throw(XABORT_TO_TOPLEVEL);
             }

         }
         return _this_;
}

qtree* group_munch_left(qtree* _this_, qtree* left) {
    return group_munch_right(_this_);
}



qtree* group_with_headnode_munch_left(qtree* _this_, qtree* left) {
    _this_->_headnode = left;
    return _this_->munch_right(_this_);
}


qtree* t_opn_munch_left(qtree* _this_, qtree* left) {
    _this_->_headnode = left;
    return _this_->munch_right(_this_);
}



qtree* t_obr_munch_left(qtree* _this_, qtree* left) {
    _this_->_headnode = left;
    return group_munch_right(_this_);
}



qtree* t_lsp_munch_right(qtree* _this_) {
         assert(_this_->_closer);

         long limit = 10000;
         long i = 0;

         qtree* e = 0;
         t_typ  nexty = 0;
         for(i = 0; true; ++i) {

             // check to see if we have termination next.
             nexty = _this_->_qfac->token()->_ty;
             if (nexty == _this_->_closer) {
                 _this_->_qfac->advance(_this_->_closer);
                 return _this_;
             }
             if (nexty == t_eof) {
                 return _this_;
             }

             if (i==0) {
                 _this_->_headnode = _this_->_qfac->token();
                 _this_->_qfac->advance(0);
                 continue;
             }

             e = _this_->_qfac->expression(0);

             if (!e) {
                 return _this_;
             }
             if (e->_ty == _this_->_closer) {
                 return _this_;
             }
             if (e->_ty == t_eof) {
                 return _this_;
             }

             assert(e != _this_);

             _this_->add_child(e);

             if (i == limit) {
                 printf("error in  qtree_%s::munch_right(): reached inf "
                        "loop detection of %ld iterations inside a '(': .\n",_this_->_ty, limit);
                 _this_->_qfac->print_error_context(_this_->_id);
                 l3throw(XABORT_TO_TOPLEVEL);
             }

         }
         return _this_;
}



qtree* postfix_munch_left(qtree* _this_, qtree* left) {
         _this_->add_child( _this_->_qfac->expression(_this_->_lbp));
         return _this_;
}



qtree* t_cmd_munch_left(qtree* _this_, qtree* left) {
    _this_->_headnode = left;
    return _this_->munch_right(_this_);
}


qtree* assign_munch_left(qtree* _this_, qtree* left) {
          _this_->add_child(left);
          _this_->add_child( _this_->_qfac->expression(_this_->_rbp));
          return _this_;
}


     // unary minus sign
qtree* minus_munch_right(qtree* _this_) {
         _this_->add_child( _this_->_qfac->expression(_this_->_rbp));
         return _this_;
}

qtree* minus_munch_left(qtree* _this_, qtree* left) {
          _this_->add_child(left);
          _this_->add_child( _this_->_qfac->expression(_this_->_lbp));
          return _this_;
}

 //
 //  humble ;
 //

qtree* stopper_munch_left(qtree* _this_, qtree* left) {
    return left; // needs to do this so as not to loose the accumulated tree.
}


 //
 // ? : ternary operator
 //

qtree* ternary_munch_left(qtree* _this_, qtree* left) { 
    _this_->add_child( left);
    _this_->add_child( _this_->_qfac->expression(0));
    _this_->_qfac->advance(m_cln);
    _this_->add_child( _this_->_qfac->expression(0));
    return _this_;
}


//
// while
//

qtree* while_munch_right(qtree* _this_) { 

    // first chld has the condition in parenthesis.
    t_typ got = _this_->_qfac->advance(t_opn);

    if (got == t_cpn) {
        printf("error in parsing while statement: null condition detected.\n");
        _this_->_qfac->print_error_context(_this_->_qfac->token()->_id);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    _this_->add_child(_this_->_qfac->expression(0));
    t_typ got2 = _this_->_qfac->advance(t_cpn);

    if (_this_->_qfac->eof()) {
        printf("error in parsing while statement: end-of-file before body.\n");
        _this_->_qfac->print_error_context(_this_->_id);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    if (got2 != t_obr) {
        printf("error in parsing while statement: expected open brace '{' after while condition.\n");
        _this_->_qfac->print_error_context(_this_->_qfac->token()->_id);
        l3throw(XABORT_TO_TOPLEVEL);            
    }

    // second chld has the block to do while the condition is true.
    _this_->add_child(_this_->_qfac->expression(0));
    return _this_;
}




//
// if
//
qtree* c_if_munch_right(qtree* _this_) { 
    
    // first chld has the condition in parenthesis.
    t_typ got = _this_->_qfac->advance(t_opn);
    
    if (got == t_cpn) {
        printf("error in parsing if statement: null condition detected.\n");
        _this_->_qfac->print_error_context(_this_->_qfac->token()->_id);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    _this_->add_child(_this_->_qfac->expression(0));
    _this_->_qfac->advance(t_cpn);
    
    if (_this_->_qfac->eof()) {
        printf("error in parsing if statement: end-of-file before then-body.\n");
        _this_->_qfac->print_error_context(_this_->_id);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    // second chld has the statement/block to do if the condition is true.
    _this_->add_child(_this_->_qfac->expression(0));
    
    while (!_this_->_qfac->eof() && _this_->_qfac->token()->_ty == t_semicolon) {
        _this_->_qfac->advance(t_semicolon);
    }
    
    // check for else
    if (_this_->_qfac->token()->_ty == t_c_else) {
        _this_->add_child(_this_->_qfac->expression(0));
    }
    
    return _this_;
}
    


//
// for loop : t_c_for
//


qtree* c_for_munch_right(qtree* _this_) {

        // first chld has the condition in parenthesis.
        if (_this_->_qfac->token()->_ty != t_opn) {
            printf("error in parsing for statement: no condition detected.\n");
            _this_->_qfac->print_error_context(_this_->_qfac->token()->_id);
            l3throw(XABORT_TO_TOPLEVEL);
        }

        // grab the stuff in the for( )  parenthesis. skip past the paren so
        //  it doesn't treat this like a function call.
        _this_->_qfac->advance(t_opn);

         long limit = 6;
         long i = 0;

         qtree* e = 0;
         while (1) {
             // check to see if we have termination next.
             if (_this_->_qfac->token()->_ty == t_cpn) {
                 _this_->_qfac->advance(t_cpn);
                 break;
             }

             e = _this_->_qfac->expression(0);
             if (!e) break;
             if (e->_ty == t_cpn) break;

             assert(e != _this_);
             if (e->_ty != t_semicolon) {
                 _this_->add_child(e);
             }

             if (++i == limit) {
                 printf("error in parsing for() loop parenthesis: too many statements inside (); saw %ld statements.\n",limit);
                 _this_->_qfac->print_error_context(_this_->_qfac->token()->_id);
                 l3throw(XABORT_TO_TOPLEVEL);
             }
         }


        if (_this_->_qfac->eof()) {
            printf("error in parsing for statement: end-of-file before body.\n");
            _this_->_qfac->print_error_context(_this_->_id);
            l3throw(XABORT_TO_TOPLEVEL);
        }

        // second chld has the block to do for the condition is true.
        _this_->_body = _this_->_qfac->expression(0);
        return _this_;
    }



//
// ++ m_ppl
// 

qtree* plusplus_munch_left(qtree* _this_, qtree* left) {
        if (left->_ty != t_semicolon && left->_ty != t_opn) {
            _this_->_postfix = true;
            _this_->add_child(left); // should we do both??? no.

            return _this_; // postfix has priority over prefix
        }
        return _this_->munch_right(_this_);
}

qtree* plusplus_munch_right(qtree* _this_) {
        // hackish... what is a better way...?
        if (_this_->_postfix) return _this_;
        qtree* t = _this_->_qfac->token();
        if (t->_ty != t_semicolon && t->_ty != t_cpn && t->_ty != t_comma && t->_ty != t_cbr && t->_ty != t_csq) {
            _this_->_prefix = true;
            _this_->add_child(_this_->_qfac->expression(_this_->_rbp));
        }
        return _this_;
}


//
// t_cppcomment
//


// consume comment
//
qtree* t_cppcomment_munch_right(qtree* _this_) {

        qtree* t = _this_->_qfac->token();

        while(!_this_->_qfac->eof() && t->_ty != t_newline) {
            _this_->_qfac->advance(0);
            t = _this_->_qfac->token();
        }

        return _this_;
}


//
// """ t_q3q triple quotes, go until next triple quote, asking for more. Also used by t_sqo.
//
qtree* t_q3q_munch_right(qtree* _this_) {
         assert(_this_->_closer);

         // allow longer texts... 30K
         long limit = 30000;
         long i = 0;

         t_typ  nexty = 0;
         while (1) {

             // check to see if we have termination next.
             nexty = _this_->_qfac->token()->_ty;


             if (nexty == _this_->_closer) {
                 _this_->_val.e = _this_->_qfac->token()->_val.e;

                 _this_->_qfac->advance(_this_->_closer);
                 return _this_;
             }

             if (nexty == t_eof) {
                 return _this_;
             }

             _this_->_qfac->advance(0);

             if (++i == limit) {
                 printf("error in  qtree_%s::munch_right(): reached inf "
                        "loop detection of %ld iterations inside a begin/end token set: .\n",_this_->_ty, limit);
                 _this_->_qfac->print_error_context(_this_->_id);
                 l3throw(XABORT_TO_TOPLEVEL);
             }

         }
         return _this_;
}
