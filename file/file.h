#ifndef FILE_H
#define FILE_H

extern int    file_exists(const char *name);
extern size_t file_size(const char *name);
extern int    file_isdir(const char *dir);
extern int    file_cp(const char *src, const char *dst);
extern int    file_mv(const char *old, const char *new);
extern int    file_backup_mv(const char *old, const char *new);
extern int    file_rm(const char *name);
extern int    file_mkdir(const char *dir);

#endif /* !FILE_H */
