///////////////////////////////////////////////////////////////
//                                                           //
// Propeller Spin/PASM Compiler Command Line Tool 'OpenSpin' //
// (c)2012-2015 Parallax Inc.                                //
// Adapted from Jeff Martin's Delphi code by Roy Eltham      //
// See end of file for terms of use.                         //
//                                                           //
///////////////////////////////////////////////////////////////
//
// openspin.cpp
//
//
#include "openspin.h"



CompilerData* s_pCompilerData = NULL;
int  s_nObjStackPtr = 0;
int  s_nFilesAccessed = 0;
char s_filesAccessed[MAX_FILES][PATH_MAX];
bool s_bUnusedMethodElimination = true;

FILE* OpenFileInPath(const char *name, const char *mode)
{
    const char* pTryPath = NULL;

    FILE* file = fopen(name, mode);
    if (!file)
    {
        PathEntry* entry = NULL;
        while(!file)
        {
            pTryPath = MakeNextPath(&entry, name);
            if (pTryPath)
            {
                file = fopen(pTryPath, mode);
                if (file != NULL)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    if (s_nFilesAccessed < MAX_FILES)
    {
        if (!pTryPath)
        {
#ifdef WIN32
            if (_fullpath(s_filesAccessed[s_nFilesAccessed], name, PATH_MAX) == NULL)
#else
            if (realpath(name, s_filesAccessed[s_nFilesAccessed]) == NULL)
#endif
            {
                strcpy(s_filesAccessed[s_nFilesAccessed], name);
            }
            s_nFilesAccessed++;
        }
        else
        {
            strcpy(s_filesAccessed[s_nFilesAccessed++], pTryPath);
        }
    }
    else
    {
        // should never hit this, but just in case
        printf("Too many files!\n");
        exit(-2);
    }

    return file;
}

// returns NULL if the file failed to open or is 0 length
char* LoadFile(char* pFilename, int* pnLength)
{
    char* pBuffer = 0;

    FILE* pFile = OpenFileInPath(pFilename, "rb");
    if (pFile != NULL)
    {
        // get the length of the file by seeking to the end and using ftell
        fseek(pFile, 0, SEEK_END);
        *pnLength = ftell(pFile);

        if (*pnLength > 0)
        {
            pBuffer = (char*)malloc(*pnLength+1); // allocate a buffer that is the size of the file plus one char
            pBuffer[*pnLength] = 0; // set the end of the buffer to 0 (null)

            // seek back to the beginning of the file and read it in
            fseek(pFile, 0, SEEK_SET);
            fread(pBuffer, 1, *pnLength, pFile);
        }
        fclose(pFile);
    }

    return pBuffer;
}

int GetData(unsigned char* pDest, char* pFileName, int nMaxSize)
{
    int nBytesRead = 0;
    FILE* pFile = OpenFileInPath(pFileName, "rb");

    if (pFile)
    {
        if (pDest)
        {
            nBytesRead = (int)fread(pDest, 1, nMaxSize, pFile);
        }
        fclose(pFile);
    }
    else
    {
        printf("Cannot find/open dat file: %s \n", pFileName);
        return -1;
    }

    return nBytesRead;
}

bool GetPASCIISource(char* pFilename)
{
    // read in file to temp buffer, convert to PASCII, and assign to s_pCompilerData->source
    int nLength = 0;
    char* pBuffer = LoadFile(pFilename, &nLength);
    if (pBuffer)
    {
        char* pPASCIIBuffer = new char[nLength+1];
        if (!UnicodeToPASCII(pBuffer, nLength, pPASCIIBuffer, false))
        {
            printf("Unrecognized text encoding format!\n");
            delete [] pPASCIIBuffer;
            free(pBuffer);
            return false;
        }

        // clean up any previous buffer
        if (s_pCompilerData->source)
        {
            delete [] s_pCompilerData->source;
        }

        s_pCompilerData->source = pPASCIIBuffer;

        free(pBuffer);
    }
    else
    {
        s_pCompilerData->source = NULL;
        return false;
    }

    return true;
}

void PrintError(const char* pFilename, const char* pErrorString)
{
    int lineNumber = 1;
    int column = 1;
    int offsetToStartOfLine = -1;
    int offsetToEndOfLine = -1;
    int offendingItemStart = 0;
    int offendingItemEnd = 0;
    GetErrorInfo(lineNumber, column, offsetToStartOfLine, offsetToEndOfLine, offendingItemStart, offendingItemEnd);

    printf("%s(%d:%d) : error : %s\n", pFilename, lineNumber, column, pErrorString);

    char errorItem[512];
    char errorLine[512];
    if ( offendingItemStart == offendingItemEnd && s_pCompilerData->source[offendingItemStart] == 0 )
    {
        strcpy(errorLine, "End Of File");
        strcpy(errorItem, "N/A");
    }
    else
    {
        strncpy(errorLine, &s_pCompilerData->source[offsetToStartOfLine], offsetToEndOfLine - offsetToStartOfLine);
        errorLine[offsetToEndOfLine - offsetToStartOfLine] = 0;

        strncpy(errorItem, &s_pCompilerData->source[offendingItemStart], offendingItemEnd - offendingItemStart);
        errorItem[offendingItemEnd - offendingItemStart] = 0;
    }

    printf("Line:\n%s\nOffending Item: %s\n", errorLine, errorItem);
}

bool CompileRecursively(char* pFilename, bool bQuiet, bool bFileTreeOutputOnly, int& nCompileIndex)
{
    nCompileIndex++;
    if (s_nObjStackPtr > 0 && (!bQuiet || bFileTreeOutputOnly))
    {
        char spaces[] = "                              \0";
        printf("%s...%s\n", &spaces[32-(s_nObjStackPtr<<1)], pFilename);
    }
    s_nObjStackPtr++;
    if (s_nObjStackPtr > ObjFileStackLimit)
    {
        printf("%s : error : Object nesting exceeds limit of %d levels.\n", pFilename, ObjFileStackLimit);
        return false;
    }

    if (!GetPASCIISource(pFilename))
    {
        printf("%s : error : Can not find/open file.\n", pFilename);
        return false;
    }

    if (!s_pCompilerData->bFinalCompile  && s_bUnusedMethodElimination)
    {
        AddObjectName(pFilename, nCompileIndex);
    }

    strcpy(s_pCompilerData->current_filename, pFilename);
    char* pExtension = strstr(s_pCompilerData->current_filename, ".spin");
    if (pExtension != 0)
    {
        *pExtension = 0;
    }

    // first pass on object
    const char* pErrorString = Compile1();
    if (pErrorString != 0)
    {
        PrintError(pFilename, pErrorString);
        return false;
    }

    if (s_pCompilerData->obj_files > 0)
    {
        char filenames[file_limit*256];

        int numObjects = s_pCompilerData->obj_files;
        for (int i = 0; i < numObjects; i++)
        {
            // copy the obj filename appending .spin if it doesn't have it.
            strcpy(&filenames[i<<8], &(s_pCompilerData->obj_filenames[i<<8]));
            if (strstr(&filenames[i<<8], ".spin") == NULL)
            {
                strcat(&filenames[i<<8], ".spin");
            }
        }

        for (int i = 0; i < numObjects; i++)
        {
            if (!CompileRecursively(&filenames[i<<8], bQuiet, bFileTreeOutputOnly, nCompileIndex))
            {
                return false;
            }
        }

        if (!GetPASCIISource(pFilename))
        {
            printf("%s : error : Can not find/open file.\n", pFilename);
            return false;
        }

        strcpy(s_pCompilerData->current_filename, pFilename);
        char* pExtension = strstr(s_pCompilerData->current_filename, ".spin");
        if (pExtension != 0)
        {
            *pExtension = 0;
        }
        pErrorString = Compile1();
        if (pErrorString != 0)
        {
            PrintError(pFilename, pErrorString);
            return false;
        }

        if (!CopyObjectsFromHeap(s_pCompilerData, filenames))
        {
            printf("%s : error : Object files exceed 128k.\n", pFilename);
            return false;
        }
    }

    // load all DAT files
    if (s_pCompilerData->dat_files > 0)
    {
        int p = 0;
        for (int i = 0; i < s_pCompilerData->dat_files; i++)
        {
            // Get DAT's Files

            // Get name information
            char filename[256];
            strcpy(&filename[0], &(s_pCompilerData->dat_filenames[i<<8]));

            // Load file and add to dat_data buffer
            s_pCompilerData->dat_lengths[i] = GetData(&(s_pCompilerData->dat_data[p]), &filename[0], data_limit - p);
            if (s_pCompilerData->dat_lengths[i] == -1)
            {
                s_pCompilerData->dat_lengths[i] = 0;
                return false;
            }
            if (p + s_pCompilerData->dat_lengths[i] > data_limit)
            {
                printf("%s : error : Object files exceed 128k.\n", pFilename);
                return false;
            }
            s_pCompilerData->dat_offsets[i] = p;
            p += s_pCompilerData->dat_lengths[i];
        }
    }

    // second pass of object
    strcpy(s_pCompilerData->current_filename, pFilename);
    pExtension = strstr(s_pCompilerData->current_filename, ".spin");
    if (pExtension != 0)
    {
        *pExtension = 0;
    }
    pErrorString = Compile2();
    if (pErrorString != 0)
    {
        PrintError(pFilename, pErrorString);
        return false;
    }

    // Check to make sure object fits into 32k (or eeprom size if specified as larger than 32k)
    unsigned int i = 0x10 + s_pCompilerData->psize + s_pCompilerData->vsize + (s_pCompilerData->stack_requirement << 2);
    if ((s_pCompilerData->compile_mode == 0) && (i > s_pCompilerData->eeprom_size))
    {
        printf("%s : error : Object exceeds runtime memory limit by %d longs.\n", pFilename, (i - s_pCompilerData->eeprom_size) >> 2);
        return false;
    }

    // save this object in the heap
    if (!AddObjectToHeap(pFilename, s_pCompilerData))
    {
        printf("%s : error : Object Heap Overflow.\n", pFilename);
        return false;
    }
    s_nObjStackPtr--;

    return true;
}

bool ComposeRAM(unsigned char** ppBuffer, int& bufferSize, bool bBinary, unsigned int eeprom_size)
{
    unsigned int varsize = s_pCompilerData->vsize;                                                // variable size (in bytes)
    unsigned int codsize = s_pCompilerData->psize;                                                // code size (in bytes)
    unsigned int pubaddr = *((unsigned short*)&(s_pCompilerData->obj[8]));                        // address of first public method
    unsigned int publocs = *((unsigned short*)&(s_pCompilerData->obj[10]));                       // number of stack variables (locals), in bytes, for the first public method
    unsigned int pbase = 0x0010;                                                                  // base of object code
    unsigned int vbase = pbase + codsize;                                                         // variable base = object base + code size
    unsigned int dbase = vbase + varsize + 8;                                                     // data base = variable base + variable size + 8
    unsigned int pcurr = pbase + pubaddr;                                                         // Current program start = object base + public address (first public method)
    unsigned int dcurr = dbase + 4 + (s_pCompilerData->first_pub_parameters << 2) + publocs;      // current data stack pointer = data base + 4 + FirstParams*4 + publocs

    if (bBinary)
    {
       // reset ram
       *ppBuffer = new unsigned char[vbase];
       memset(*ppBuffer, 0, vbase);
       bufferSize = vbase;
    }
    else
    {
       if (vbase + 8 > eeprom_size)
       {
          printf("ERROR: eeprom size exceeded by %d longs.\n", (vbase + 8 - eeprom_size) >> 2);
          return false;
       }
       // reset ram
       *ppBuffer = new unsigned char[eeprom_size];
       memset(*ppBuffer, 0, eeprom_size);
       bufferSize = eeprom_size;
       (*ppBuffer)[dbase-8] = 0xFF;
       (*ppBuffer)[dbase-7] = 0xFF;
       (*ppBuffer)[dbase-6] = 0xF9;
       (*ppBuffer)[dbase-5] = 0xFF;
       (*ppBuffer)[dbase-4] = 0xFF;
       (*ppBuffer)[dbase-3] = 0xFF;
       (*ppBuffer)[dbase-2] = 0xF9;
       (*ppBuffer)[dbase-1] = 0xFF;
    }

    // set clock frequency and clock mode
    *((int*)&((*ppBuffer)[0])) = s_pCompilerData->clkfreq;
    (*ppBuffer)[4] = s_pCompilerData->clkmode;

    // set interpreter parameters
    ((unsigned short*)&((*ppBuffer)[4]))[1] = (unsigned short)pbase;         // always 0x0010
    ((unsigned short*)&((*ppBuffer)[4]))[2] = (unsigned short)vbase;
    ((unsigned short*)&((*ppBuffer)[4]))[3] = (unsigned short)dbase;
    ((unsigned short*)&((*ppBuffer)[4]))[4] = (unsigned short)pcurr;
    ((unsigned short*)&((*ppBuffer)[4]))[5] = (unsigned short)dcurr;

    // set code
    memcpy(&((*ppBuffer)[pbase]), &(s_pCompilerData->obj[4]), codsize);

    // install ram checksum byte
    unsigned char sum = 0;
    for (unsigned int i = 0; i < vbase; i++)
    {
      sum = sum + (*ppBuffer)[i];
    }
    (*ppBuffer)[5] = (unsigned char)((-(sum+2028)) );

    return true;
}


void CleanupMemory(bool bPathsAndUnusedMethodData)
{
    // cleanup
    if ( s_pCompilerData )
    {
        delete [] s_pCompilerData->list;
        delete [] s_pCompilerData->doc;
        delete [] s_pCompilerData->obj;
        delete [] s_pCompilerData->source;
    }
    CleanObjectHeap();
    if (bPathsAndUnusedMethodData)
    {
        CleanupPathEntries();
        CleanUpUnusedMethodData();
    }
    Cleanup();
}
