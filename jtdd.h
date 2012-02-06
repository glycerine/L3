//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef _JTDD_H
#define _JTDD_H

#include <string>
#include <assert.h>

// JTDD: test driven development utility macros.

// comment or undef _JTDD_SHOW_PASS to make passing tests invisible.
#define _JTDD_SHOW_PASS

// AT macro: note where we were declared, for inspecting allocation/deallocation
// needs -std=c++0x switch to g++ for __func__ to work.
//

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define STD_STRING_WHERE (std::string(__func__) + "()  " + std::string(AT))
#define STD_STRING_GLOBAL (std::string("global/file scope") + "()  " + std::string(AT))






#ifdef _JTDD_TESTS_ON


#define JTDD_PREMAIN(my_jttd_test_name) \
   struct jtdd_##my_jttd_test_name {  \
    jtdd_##my_jttd_test_name() { \
    static char _cur_testname[] = #my_jttd_test_name " in " __FILE__ ; \


#ifdef _JTDD_SHOW_PASS

#define JTDD_END(my_jttd_test_name) \
    printf("jtdd test passed: '%s'.\n",_cur_testname); \
}  \
}; \
jtdd_##my_jttd_test_name jtdd_instance_##my_jttd_test_name;

#else

#define JTDD_END(my_jttd_test_name) \
}  \
}; \
jtdd_##my_jttd_test_name jtdd_instance_##my_jttd_test_name;

#endif


// For ease of detection of pass/fail, a successful test should be as silent as possible.
// A failed test however should loudly proclaim at least this if not more detail:
#define jassert(expr) \
    if (!(expr)) { \
        printf("test '%s' failed!\n",_cur_testname); \
        assert(0); \
    } \


//
// example/use
/* 

JTDD_PREMAIN(mytestdemo)
{

  jassert(some_expression_that_must_be_true);
}
JTDD_END(mytestdemo)

*/




#else
// _JTDD_TESTS_ON was not defined



#define JTDD_PREMAIN(my_jttd_test_name) \
void jtdd_##my_jttd_test_name() {			\
    char _cur_testname[] = #my_jttd_test_name ;

#define JTDD_END(my_jttd_test_name)  }


#endif

#endif /*  _JTDD_H */

