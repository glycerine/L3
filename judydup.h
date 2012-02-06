//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef JUDY_DUP_H
#define JUDY_DUP_H

typedef judySmap<llref*> judys_llref;

int copy_judyL(void* src, void** dest);
int copy_judySL(void* src, void** dest);

int copy_judySL_typed(judys_llref* src, judys_llref** dest);

void test_judySL();
void test_map();

#endif /* JUDY_DUP_H */

