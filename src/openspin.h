#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ObjFileStackLimit   16

#define ListLimit           2000000
#define DocLimit            2000000

#define MAX_FILES           2048    // an object can only reference 32 other objects and only 32 dat files, so the worst case is 32*32*2 files

#ifndef OPENSPIN_H
#define OPENSPIN_H

#include "PropellerCompiler.h"
#include "objectheap.h"
#include "pathentry.h"
#include "textconvert.h"
#include "preprocess.h"

#endif

extern CompilerData* s_pCompilerData;
extern char s_filesAccessed[MAX_FILES][PATH_MAX];
extern bool s_bUnusedMethodElimination;


FILE* OpenFileInPath(const char *name, const char *mode);
char* LoadFile(char* pFilename, int* pnLength);
int GetData(unsigned char* pDest, char* pFileName, int nMaxSize);
bool GetPASCIISource(char* pFilename);
void PrintError(const char* pFilename, const char* pErrorString);
bool CompileRecursively(char* pFilename, bool bQuiet, bool bFileTreeOutputOnly, int& nCompileIndex);
bool ComposeRAM(unsigned char** ppBuffer, int& bufferSize, bool bBinary, unsigned int eeprom_size);
void CleanupMemory(bool bPathsAndUnusedMethodData = true);
