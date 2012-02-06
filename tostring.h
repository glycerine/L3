//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef TOSTRING_H
#define TOSTRING_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <Judy.h>
#include "qexp.h"
#include <string>
#include <list>
#include <vector>
#include <string>
#include <tr1/unordered_map>
#include <map>
#include "l3path.h"
#include "quicktype.h"
#include "dv.h"


// to string methods
void to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void bool_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void double_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void string_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void literal_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void ptrvec_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void fun_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void hash_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void obj_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void symvec_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void sexp_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);
void dq_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);

#endif /*  TOSTRING_H */
