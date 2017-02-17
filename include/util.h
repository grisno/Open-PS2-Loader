#ifndef __UTIL_H
#define __UTIL_H

#include <gsToolkit.h>

int getFileSize(int fd);
int openFile(char* path, int mode);
void* readFile(char* path, int align, int* size);
void checkCreateDir(char* path);
int listDir(char* path, char* separator, int maxElem,
		int (*readEntry)(int index, char *path, char* separator, char* name, unsigned int mode));

typedef struct {
	int fd;
	int mode;
	char* buffer;
	unsigned int size;
	unsigned int available;
	char* lastPtr;
	short allocResult;
} file_buffer_t;

file_buffer_t* openFileBuffer(char* fpath, int mode, short allocResult, unsigned int size);
int readFileBuffer(file_buffer_t* readContext, char** outBuf);
void writeFileBuffer(file_buffer_t* fileBuffer, char* inBuf, int size);
void closeFileBuffer(file_buffer_t* fileBuffer);

inline int max(int a, int b);
inline int min(int a, int b);
int fromHex(char digit);
char toHex(int digit);

#endif
