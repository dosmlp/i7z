#include "i7z.h"
static WRes MyCreateDir(const UInt16 *name)
{
  return CreateDirectoryW(name, NULL) ? 0 : GetLastError();
}
static WRes OutFile_OpenUtf16(CSzFile *p, const UInt16 *name)
{
  return OutFile_OpenW(p, name);
}
int Extra7zFromMem(const WCHAR *target_path, char *data, size_t size)
{
    CFileInStream archiveStream;
    CLookToRead lookStream;
    CSzArEx db;
    SRes res;
    ISzAlloc allocImp;
    ISzAlloc allocTempImp;
    UInt16 *temp = NULL;
    size_t tempSize = 0;
    size_t targetSize = wcslen(target_path);

    allocImp.Alloc = SzAlloc;
    allocImp.Free = SzFree;

    allocTempImp.Alloc = SzAllocTemp;
    allocTempImp.Free = SzFreeTemp;

    archiveStream.file.pdata = data;
    archiveStream.file.pos = 0;
    archiveStream.file.size = size;

    FileInStream_CreateVTable(&archiveStream);
    LookToRead_CreateVTable(&lookStream, False);

    lookStream.realStream = &archiveStream.s;
    LookToRead_Init(&lookStream);

    CrcGenerateTable();

    SzArEx_Init(&db);

    res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);

    if (res == SZ_OK)
    {

        if (res == SZ_OK)
        {
            Int32 i;

            /*
            if you need cache, use these 3 variables.
            if you use external function, you can make these variable as static.
            */
            UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
            Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
            size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

            for (i = 0; i < db.NumFiles; ++i)
            {
                size_t offset = 0;
                size_t outSizeProcessed = 0;
                size_t len;
                unsigned isDir = SzArEx_IsDir(&db, i);

                len = SzArEx_GetFileNameUtf16(&db, i, NULL);
                if (len > tempSize)
                {
                    SzFree(NULL, temp);
                    tempSize = len;
                    temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
                    if (!temp)
                    {
                    res = SZ_ERROR_MEM;
                    break;
                    }
                }
                SzArEx_GetFileNameUtf16(&db, i, temp);

                wchar_t fullname[512];
                memset(fullname,0,sizeof(fullname));
                wcscpy(fullname,target_path);
                wcscat(fullname,"/");
                wcscat(fullname,temp);
                if (res != SZ_OK)
                    break;

                UInt16* name = temp;
                int j;
                for (j = 0; name[j] != 0; j++)
                {
                    if (name[j] == '/')
                    {
                        name[j] = 0;
                        wchar_t tname[512];
                        memset(tname,0,sizeof(fullname));
                        wcscpy(tname,target_path);
                        wcscat(tname,"/");
                        wcscat(tname,name);
                        MyCreateDir(tname);
                        name[j] = CHAR_PATH_SEPARATOR;
                    }
                }

                if (isDir)
                {
                    MyCreateDir(fullname);
                    continue;
                }
                else
                {
                    res = SzArEx_Extract(&db, &lookStream.s, i,
                                         &blockIndex, &outBuffer, &outBufferSize,
                                         &offset, &outSizeProcessed,
                                         &allocImp, &allocTempImp);
                    if(res != SZ_OK)
                        break;
                }

                CSzFile outFile;
                size_t processedSize;

                if(OutFile_OpenUtf16(&outFile, fullname))//创建输出文件
                {
                    printf("can not open output file\n");
                    res = SZ_ERROR_FAIL;
                    break;
                }
                processedSize = outSizeProcessed;

                if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
                {
                    printf("can not write output file\n");
                    res = SZ_ERROR_FAIL;
                    break;
                }

                if (File_Close(&outFile))
                {
                    printf("can not close output file");
                    res = SZ_ERROR_FAIL;
                    break;
                }

                if (SzBitWithVals_Check(&db.Attribs, i))
                    SetFileAttributesW(fullname, db.Attribs.Vals[i]);
            }//end for
            IAlloc_Free(&allocImp, outBuffer);
        }
    }

    SzArEx_Free(&db, &allocImp);
    SzFree(NULL, temp);

    if (res == SZ_OK)
    {
        return 0;
    }

    if (res == SZ_ERROR_UNSUPPORTED)
    {
        printf("decoder doesn't support this archive");
    }
    else if (res == SZ_ERROR_MEM)
    {
        printf("can not allocate memory");
    }
    else if (res == SZ_ERROR_CRC)
    {
        printf("CRC error");
    }
    else
    {
        printf("\nERROR #%d\n", res);
    }

    return res;

}
