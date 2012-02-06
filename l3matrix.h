//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3MATRIX_H
#define L3MATRIX_H

#include "terp.h"
#include <vector>

struct _l3obj;
struct Tag;

// to query a matrix and remember shape: an Index
struct Ix {
    const static int  dimmax = 2;
    long _dim[dimmax]; // the shape: row, col, z, ...

    Ix() {
        _dim[0]=0;
        _dim[1]=0;
    }
    Ix(long i, long j) {
        _dim[0]=i;
        _dim[1]=j;
    }
    Ix(long i) {
        _dim[0]=i;
        _dim[1]=0;
    }
};

struct matrixobj {
    Ix _dim;
    Ix _offset;

    void set_dim(Ix dim) {
        _dim = dim;
        _offset._dim[0]=1;
        for (int j = 1; j < Ix::dimmax; ++j) {
            _offset._dim[j] = _dim._dim[j] * _offset._dim[j-1];
        }
    }
};


// return the value of JudyL[i] as a double
double double_get(_l3obj* obj, long i);

// set the value of JudyL[i] as a double
_l3obj* double_set(_l3obj* obj, long i, double d);


// get the number of doubles stored
long double_size(_l3obj* obj);


// returns false if empty vector, else true
bool double_first(_l3obj* obj, double* val, long* index);

// return false if no more after the supplied *index value
//
bool double_next(_l3obj* obj, double* val, long* index);

// simple linear search
bool double_search(_l3obj* obj, double needle);

void double_print(_l3obj* obj, const char* indent);

void double_copy(_l3obj* obj, Tag* owner, _l3obj** retval);

bool double_is_sparse(_l3obj* obj);

void double_set_sparse(_l3obj* obj);


void double_set_dense(_l3obj* obj);



// if isnan(d) then return a size 0 vector.
_l3obj* make_new_double_obj(double d, Tag* owner, const char* varname);

L3FORWDECL(double_cpctor)
L3FORWDECL(double_dtor);

double  matrix_idx_get(l3obj* obj, Ix i);
void    matrix_idx_set(l3obj* obj, Ix i, double d);


#endif /* L3MATRIX_H */

