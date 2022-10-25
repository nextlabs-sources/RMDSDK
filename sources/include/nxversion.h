#ifndef __NXVERSION_H__
#define __NXVERSION_H__



#include "nxversionnum.h"
#if !defined(VERSION_MAJOR) || !defined(VERSION_MINOR) || !defined(BUILD_NUMBER)
#error "ERROR: version numbers are not defined by nxversionnum.h!"
#endif



#define VER_FILEVERSION                                 \
  VERSION_MAJOR,VERSION_MINOR,BUILD_NUMBER
#define VER_PRODUCTVERSION                              \
  VERSION_MAJOR,VERSION_MINOR,BUILD_NUMBER



#define VERSION_STR_BASE(maj,min,build)                 \
  #maj "." #min "." #build
#define VERSION_STR(maj,min,build)                      \
  VERSION_STR_BASE(maj,min,build)

#define VER_FILEVERSION_STR                             \
  VERSION_STR(VERSION_MAJOR,VERSION_MINOR,BUILD_NUMBER)
#define VER_PRODUCTVERSION_STR                          \
  VERSION_STR(VERSION_MAJOR,VERSION_MINOR,BUILD_NUMBER)



#define VER_PRODUCTNAME_STR                             \
  "SkyDRM"

#define VER_LEGALCOPYRIGHT_STR                          \
  "Copyright (C) 2017-2022 NextLabs, Inc.  All rights reserved."


#define VER_LEGALCOPYRIGHT_STR_SYSLINK                    \
  "Copyright (C) 2017-2022 <a>NextLabs, Inc.</a>  All rights reserved."


#endif  // __NXVERSION_H__
