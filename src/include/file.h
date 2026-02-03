#ifndef _FILE_H_
#define _FILE_H_

// file.c
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, data, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
char *file_loadfile_check_tek(int clustno, int *psize, int *fat);

// tek.c
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

#endif // _FILE_H_
