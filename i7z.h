#ifndef __I7Z_H
#define __I7Z_H

#include "Precomp.h"

#include <stdio.h>
#include <string.h>

#include "7z.h"
#include "7zAlloc.h"
#include "7zBuf.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zVersion.h"
EXTERN_C_BEGIN
    __declspec(dllexport) int Extra7zFromMem(const WCHAR* target_path,char* data,size_t size);
EXTERN_C_END
#endif
