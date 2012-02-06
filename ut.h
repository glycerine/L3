//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef UNITTEST_H
#define UNITTEST_H

#include "l3path.h"

namespace LibEditLine {
#include <histedit.h>
}

struct _l3obj;
class jmemlogger;
class Tag;

struct unittest {

    unittest();

    long _max_test;
    long _last_test;

    void pick_test(long testnum, l3path* lineout);

    long last_test();  // last invoked test number

    long max_test();   // highest test number we know

    long run_next_test(); 
};

typedef std::list<Tag*>     list_itag;
typedef list_itag::iterator  list_itag_it;

// track historical commands, releasing those that are old
class cmdhistory {
 public:

    void teardown();

    // set *pvalue=0; if we delete it.
    void last_cmd_had_value(l3path* cmd, _l3obj** pvalue, Tag* owner);

    void keep_last(long ncmds_as_history);
    long keeping_last();
    void show_history();

    void add(const char* msg);
    void add_sync(const char* msg);
    void monitor_this_tag(Tag* pt);
    void sync();

    void  init_editline_history();
    void teardown_editline_history();

    void   add_editline_history(const char *line);
    char*  readline(const char *prompt);

    char* get_prompt() { return pPrompt; }
    void  set_prompt(char* p) { pPrompt = p; }

    void init(l3path path);
    ~cmdhistory();

 private:

    jmemlogger*  _histlog;

    std::list<_l3obj*> _history_list;
    _l3obj* _history_last;
    long   _history_len;
    long   _history_max; // 0 => keep everything.
    std::list<Tag*> _monitor_tags;
    Tag*  _monitor_tag;

    LibEditLine::EditLine* _editline;
    LibEditLine::History*  _editline_history;

    char* pPrompt;
};



#endif /*  UNITTEST_H */

