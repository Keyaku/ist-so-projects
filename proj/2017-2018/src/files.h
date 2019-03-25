#ifndef FILES_H
#define FILES_H

/* Material de manuseamento de ficheiros */
#define F_EXISTS 1
#define F_NOT_EXISTS 0
#define F_ERROR -1

int file_exists(const char* filename);
void file_delete(const char *filename);

#endif // FILES_H
