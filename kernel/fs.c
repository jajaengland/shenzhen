#include "fs.h"
#include "heap.h"
#include "kprintf.h"

static fs_node_t *root_node = 0;
static fs_node_t *cwd_node  = 0;

/* simple string functions */
static int fs_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void fs_strncpy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int fs_strlen(const char *s) {
    int n = 0; while (*s++) n++; return n;
}

static void fs_memcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = dst; const uint8_t *s = src;
    while (n--) *d++ = *s++;
}

static fs_node_t *alloc_node(void) {
    fs_node_t *n = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    if (!n) return 0;
    /* zero it out */
    uint8_t *p = (uint8_t *)n;
    for (uint32_t i = 0; i < sizeof(fs_node_t); i++) p[i] = 0;
    return n;
}

void fs_init(void) {
    root_node = alloc_node();
    if (!root_node) { kprintf("fs: failed to allocate root\n"); return; }
    root_node->name[0] = '/';
    root_node->name[1] = '\0';
    root_node->type    = FS_DIR;
    root_node->parent  = root_node;  /* root's parent is itself */
    cwd_node = root_node;
    kprintf("FS: ramfs initialized\n");
}

fs_node_t *fs_root(void) { return root_node; }
fs_node_t *fs_cwd(void)  { return cwd_node; }
void fs_set_cwd(fs_node_t *dir) { if (dir && dir->type == FS_DIR) cwd_node = dir; }

fs_node_t *fs_find(fs_node_t *dir, const char *name) {
    if (!dir || dir->type != FS_DIR) return 0;
    if (fs_strcmp(name, ".") == 0)  return dir;
    if (fs_strcmp(name, "..") == 0) return dir->parent;
    fs_node_t *child = dir->children;
    while (child) {
        if (fs_strcmp(child->name, name) == 0) return child;
        child = child->next;
    }
    return 0;
}

static void add_child(fs_node_t *parent, fs_node_t *child) {
    child->parent = parent;
    child->next   = parent->children;
    parent->children = child;
}

fs_node_t *fs_mkdir(fs_node_t *parent, const char *name) {
    if (!parent || parent->type != FS_DIR) return 0;
    if (fs_find(parent, name)) return 0;  /* already exists */
    fs_node_t *node = alloc_node();
    if (!node) return 0;
    fs_strncpy(node->name, name, FS_NAME_MAX);
    node->type = FS_DIR;
    add_child(parent, node);
    return node;
}

fs_node_t *fs_mkfile(fs_node_t *parent, const char *name) {
    if (!parent || parent->type != FS_DIR) return 0;
    if (fs_find(parent, name)) return 0;  /* already exists */
    fs_node_t *node = alloc_node();
    if (!node) return 0;
    fs_strncpy(node->name, name, FS_NAME_MAX);
    node->type = FS_FILE;
    add_child(parent, node);
    return node;
}

int fs_write(fs_node_t *file, const void *data, uint32_t size) {
    if (!file || file->type != FS_FILE) return -1;
    if (size > FS_MAX_DATA) size = FS_MAX_DATA;
    fs_memcpy(file->data, data, size);
    file->size = size;
    return (int)size;
}

int fs_read(fs_node_t *file, void *buf, uint32_t size) {
    if (!file || file->type != FS_FILE) return -1;
    if (size > file->size) size = file->size;
    fs_memcpy(buf, file->data, size);
    return (int)size;
}

int fs_delete(fs_node_t *node) {
    if (!node || node == root_node) return -1;
    if (node->type == FS_DIR && node->children) return -1;  /* not empty */

    /* remove from parent's child list */
    fs_node_t *parent = node->parent;
    if (parent->children == node) {
        parent->children = node->next;
    } else {
        fs_node_t *prev = parent->children;
        while (prev && prev->next != node) prev = prev->next;
        if (prev) prev->next = node->next;
    }

    /* if we're deleting cwd, go up */
    if (cwd_node == node) cwd_node = parent;

    kfree(node);
    return 0;
}

/* resolve a path (absolute or relative) */
fs_node_t *fs_resolve(const char *path) {
    if (!path || !path[0]) return cwd_node;

    fs_node_t *cur = (path[0] == '/') ? root_node : cwd_node;
    if (fs_strcmp(path, "/") == 0) return root_node;

    /* skip leading slash */
    if (path[0] == '/') path++;

    char part[FS_NAME_MAX];
    while (*path) {
        /* extract next component */
        int i = 0;
        while (*path && *path != '/') part[i++] = *path++;
        part[i] = '\0';
        if (*path == '/') path++;
        if (!part[0]) continue;

        cur = fs_find(cur, part);
        if (!cur) return 0;
    }
    return cur;
}

/* build current path string */
void fs_pwd(char *buf, uint32_t len) {
    if (!buf || !len) return;
    if (cwd_node == root_node) { buf[0] = '/'; buf[1] = '\0'; return; }

    /* walk up to root collecting names */
    char parts[16][FS_NAME_MAX];
    int  depth = 0;
    fs_node_t *n = cwd_node;
    while (n != root_node && depth < 16) {
        fs_strncpy(parts[depth++], n->name, FS_NAME_MAX);
        n = n->parent;
    }

    /* build path in reverse */
    uint32_t pos = 0;
    for (int i = depth - 1; i >= 0 && pos < len - 1; i--) {
        buf[pos++] = '/';
        for (int j = 0; parts[i][j] && pos < len - 1; j++)
            buf[pos++] = parts[i][j];
    }
    buf[pos] = '\0';
}
