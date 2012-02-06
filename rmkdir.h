//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef RECURSIVE_MKDIR
#define RECURSIVE_MKDIR

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

int recursive_mkdir(const char *dir, mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO);

enum file_ty { FILE_DOES_NOT_EXIST, DIR, REGFILE, SYMLINK, OTHER };

bool file_exists(const char* path, file_ty* ty = 0, long* sz = 0);

// check if the file or directory that the sym links points at actually exists.
//  so now we use stat, above we used lstat.
bool symlink_target_exists(const char* path, file_ty* ty);

// l3throw if these paths are ready to write / read. Else, return open() fd if the path is good.
int prep_out_path(char* outpath);

int prep_in_path(char* inpath);



#endif /* RECURSIVE_MKDIR */
