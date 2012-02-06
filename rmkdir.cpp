//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#include <unistd.h>
#include <limits.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "autotag.h"
#include "l3obj.h"
#include "terp.h"
#include "l3path.h"
#include "user_xcep.h"
#include "rmkdir.h"


/////////////////////// end of includes


// enum file_ty { FILE_DOES_NOT_EXIST, DIR, REGFILE, SYMLINK, OTHER };

bool file_exists(const char* path, file_ty* ty, long* sz) {

  struct stat buf; 
  int rc = lstat(path, &buf);
  if (-1 == rc) {
    if (ty) { *ty = FILE_DOES_NOT_EXIST; }
    return false;
  }

  if (sz != 0) { *sz = buf.st_size; } // sz will not include the trailing null for symlink content strings.
  
  if (S_ISDIR(buf.st_mode)) { // directory
    if (ty) *ty = DIR;
    return true;
  }

  if (S_ISLNK(buf.st_mode)) { // symbolic link? (Not in POSIX.1-1996.) -> so use lstat instead of stat.
    if (ty) *ty = SYMLINK;
    return true;
  }

  if (S_ISREG(buf.st_mode)) { // regular file
    if (ty) *ty = REGFILE;
    return true;
  }

  if (ty) *ty = OTHER;
  return true;
}

// check if the file or directory that the sym links points at actually exists.
//  so now we use stat, above we used lstat.
bool symlink_target_exists(const char* path, file_ty* ty) {

  struct stat buf; 
  int rc = stat(path, &buf);
  if (-1 == rc) {
    if (ty) { *ty = FILE_DOES_NOT_EXIST; }
    return false;
  }

  if (S_ISDIR(buf.st_mode)) { // directory
    if (ty) *ty = DIR;
    return true;
  }

  if (S_ISLNK(buf.st_mode)) { // symbolic link? (Not in POSIX.1-1996.) -> so use lstat instead of stat.
    if (ty) *ty = SYMLINK;
    return true;
  }

  if (S_ISREG(buf.st_mode)) { // regular file
    if (ty) *ty = REGFILE;
    return true;
  }

  if (ty) *ty = OTHER;
  return true;
}




int recursive_mkdir(const char *dir, mode_t mode) {
  char tmp[PATH_MAX+1];
  bzero(tmp,PATH_MAX+1);
  char *p = NULL;
  size_t len;
  int ret = 0;
  file_ty ty;



  snprintf(tmp, PATH_MAX,"%s",dir);
  len = strlen(tmp);
  if(tmp[len - 1] == '/')
    tmp[len - 1] = 0;
  for(p = tmp + 1; *p; p++)
    if(*p == '/') {
      *p = 0;
      if (!file_exists(tmp,&ty)) {
	ret = mkdir(tmp, mode);
	if (ret) return ret;
      }
      *p = '/';
    }
  if (!file_exists(tmp,&ty)) {
    ret = mkdir(tmp, mode);
  }
  return ret;
}


int prep_out_path(char* outpath) {

    int fd = 0;

    l3path  filepath(outpath);
    l3path  outdir(outpath);
    
    outdir.pop(); // get just the dir

    if (outdir.len()) {
        // does dir already exists?
        file_ty filety = FILE_DOES_NOT_EXIST;
        long    filesz = 0;
        bool    fexists = file_exists(outdir(), &filety, &filesz);

        if (fexists && filety == DIR) {
            // cool. good to go, just use this dir.

        } else {
            // make dir
            if (recursive_mkdir(outdir(), S_IRWXU | S_IRWXG | S_IRWXO)) {
                fprintf(stderr,"error in prep_out_path(): could not make directory '%s'; error is: '%s'\n",outdir(),strerror(errno));
                l3throw(XABORT_TO_TOPLEVEL);
            }
        }
    }

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH ;
    fd = open(filepath(), O_WRONLY|O_CREAT|O_TRUNC, mode);
    if (-1 == fd) {
        fprintf(stderr,"error in prep_out_path(): could not open file '%s' for writing; error is: '%s'\n",filepath(),strerror(errno));
        l3throw(XABORT_TO_TOPLEVEL);        
    }

    return fd;
}

int prep_in_path(char* inpath) {

    // confirm we got a non-zero file

    file_ty filety = FILE_DOES_NOT_EXIST;
    long    filesz = 0;
    bool    fexists = file_exists(inpath, &filety, &filesz);

    if (!fexists || filety != REGFILE || filesz <= 0) {
        printf("error: on attempting to reading file path '%s'.\n", inpath);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    int fd = open(inpath, O_RDONLY);         
    
    if (-1 == fd) {
        fprintf(stderr,"error in prep_out_path(): could not open file '%s' for writing; error is: '%s'\n",inpath,strerror(errno));
        l3throw(XABORT_TO_TOPLEVEL);
    }

    return fd;
}
