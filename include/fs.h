#pragma once
#include <stdint.h>
#include <stddef.h>

#define FS_NAME_MAX   64
#define FS_MAX_NODES  128
#define FS_MAX_DATA   4096   /* max file size in bytes */

typedef enum {
    FS_FILE = 1,
    FS_DIR  = 2,
} fs_type_t;

typedef struct fs_node {
    char           name[FS_NAME_MAX];
    fs_type_t      type;
    uint32_t       size;
    uint8_t        data[FS_MAX_DATA];  /* file contents */
    struct fs_node *parent;
    struct fs_node *children;          /* linked list of children (dirs) */
    struct fs_node *next;              /* next sibling */
} fs_node_t;

void        fs_init(void);
fs_node_t  *fs_root(void);
fs_node_t  *fs_cwd(void);
void        fs_set_cwd(fs_node_t *dir);

fs_node_t  *fs_mkdir(fs_node_t *parent, const char *name);
fs_node_t  *fs_mkfile(fs_node_t *parent, const char *name);
fs_node_t  *fs_find(fs_node_t *dir, const char *name);
fs_node_t  *fs_resolve(const char *path);   /* resolve absolute or relative path */
int         fs_write(fs_node_t *file, const void *data, uint32_t size);
int         fs_read(fs_node_t *file, void *buf, uint32_t size);
int         fs_delete(fs_node_t *node);
void        fs_pwd(char *buf, uint32_t len); /* get current path as string */
