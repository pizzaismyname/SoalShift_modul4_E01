#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>

const char *dirpath = "/home/rifqi/shift4";
const char *cipher = "qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";

struct videoSplit
{
    char filename[256];
    int max;
};

int remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d)
    {
        struct dirent *p;
        r = 0;
        while (!r && (p = readdir(d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);
    return r;
}

int strstart(const char *s, const char *t)
{
    char lastl[strlen(t) + 1];
    memcpy(lastl, s, strlen(t));
    lastl[strlen(t)] = '\0';
    return (strcmp(lastl, t) == 0 ? 1 : 0);
}

int strend(const char *s, const char *t)
{
    char lastl[strlen(t)];
    int delta = strlen(s) - strlen(t);
    sprintf(lastl, "%s", s + delta);
    return (strcmp(lastl, t) == 0 ? 1 : 0);
}

char *rot(const char *a, int enc)
{
    char *strtorotate = malloc(strlen(a) * sizeof(char) + 1);
    sprintf(strtorotate, "%s", a);
    for (int i = 0; i < strlen(strtorotate); i++)
    {
        if (strtorotate[i] == '\0')
            break;
        for (int j = 0; j < strlen(cipher); j++)
        {
            if (cipher[j] == strtorotate[i])
            {
                if (enc == 1)
                {
                    strtorotate[i] = cipher[(j + 17) % strlen(cipher)];
                }
                else
                {
                    int r = (j - 17);
                    if (r < 0)
                    {
                        strtorotate[i] = cipher[strlen(cipher) + r];
                    }
                    else
                    {
                        strtorotate[i] = cipher[r % strlen(cipher)];
                    }
                }
                break;
            }
        }
    }
    return strtorotate;
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    char dname[256];
    sprintf(dname, "%s%s", dirpath, rot(path, 1));
    if (strend(path, "/."))
    {
        dname[strlen(dname) - 2] = '.';
    }
    else if (strend(path, "/.."))
    {
        dname[strlen(dname) - 2] = '.';
        dname[strlen(dname) - 3] = '.';
    }

    printf("###LOOKING A %s\n", dname);
    int res;

    res = lstat(dname, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    char dname[256];
    sprintf(dname, "%s%s", dirpath, rot(path, 1));
    if (strend(path, "/."))
    {
        dname[strlen(dname) - 2] = '.';
    }
    else if (strend(path, "/.."))
    {
        dname[strlen(dname) - 2] = '.';
        dname[strlen(dname) - 3] = '.';
    }

    printf("################################################\n");
    printf("###LOOKING D %s\n", dname);
    DIR *dp;
    struct dirent *de;

    (void)offset;
    (void)fi;

    dp = opendir(dname);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        int v = strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 ? 1 : 0;
        if (strcmp(path, "/") == 0 && !v)
        {
            //add extra layer to filter video
            char *p = strtok(rot(de->d_name, 0), ".");
            int filter = 0;
            while (p != NULL)
            {
                if (strcmp(p, "mp4") == 0 || strcmp(p, "mkv") == 0)
                {
                    filter = 1;
                    break;
                }
                else
                {
                    p = strtok(NULL, ".");
                }
            }
            if (filter)
                continue;
        }
        if (filler(buf, v ? de->d_name : rot(de->d_name, 0), &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = mkdir(a, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_access(const char *path, int mask)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = access(a, mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
    if (S_ISREG(mode))
    {
        res = open(a, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    }
    else if (S_ISFIFO(mode))
        res = mkfifo(a, mode);
    else
        res = mknod(a, mode, rdev);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_unlink(const char *path)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = unlink(a);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rmdir(const char *path)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = rmdir(a);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    int res;

    printf("#REN %s -> %s\n", from, to);

    return -errno;

    res = rename(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = chmod(a, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = lchown(a, uid, gid);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = truncate(a, size);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = open(a, fi->flags);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int xmp_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = open(a, fi->flags, mode);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    int fd;
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    if (fi == NULL)
        fd = open(a, O_RDONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == NULL)
        close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    (void)fi;
    if (fi == NULL)
        fd = open(a, O_WRONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == NULL)
        close(fd);
    return res;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
    (void)path;
    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));
    close(fi->fh);
    return 0;
}

static int xmp_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags)
{
    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));
    int res = lsetxattr(a, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
                        size_t size)
{
    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));
    int res = lgetxattr(a, name, value, size);
    if (res == -1)
        return -errno;
    return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));
    int res = llistxattr(a, list, size);
    if (res == -1)
        return -errno;
    return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));
    int res = lremovexattr(a, name);
    if (res == -1)
        return -errno;
    return 0;
}

static struct fuse_operations xmp_oper = {
    // .readlink	= xmp_readlink,
    //.symlink	= xmp_symlink,
    //.link = xmp_link,
    .getattr = xmp_getattr,
    .access = xmp_access,
    .readdir = xmp_readdir,
    .mknod = xmp_mknod,
    .mkdir = xmp_mkdir,
    .unlink = xmp_unlink,
    .rmdir = xmp_rmdir,
    .rename = xmp_rename,
    .chmod = xmp_chmod,
    .chown = xmp_chown,
    .truncate = xmp_truncate,
    .open = xmp_open,
    .create = xmp_create,
    .read = xmp_read,
    .write = xmp_write,
    .release = xmp_release,
    .setxattr = xmp_setxattr,
    .getxattr = xmp_getxattr,
    .listxattr = xmp_listxattr,
    .removexattr = xmp_removexattr,
};

void removeVideos()
{
    char d[256];
    sprintf(d, "%s/%s", dirpath, rot("Videos", 1));
    remove_directory(d);
}

void *videoThread()
{
    char d[256];
    sprintf(d, "%s/%s", dirpath, rot("Videos", 1));
    mkdir(d, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    struct dirent *de;

    DIR *dp = opendir(dirpath);
    if (dp != NULL)
    {
        //Reading all video files in `dirpath` that have the same prefix
        struct videoSplit v[10];
        int c = 0;
        while ((de = readdir(dp)) != NULL)
        {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
            char *p;
            p = strtok(rot(de->d_name, 0), ".");
            char first[256];
            sprintf(first, "%s", p);
            while (p != NULL)
            {
                if (strcmp(p, "mp4") == 0 || strcmp(p, "mkv") == 0)
                {
                    char fname[1000];
                    sprintf(fname, "%s.%s", first, p);
                    //Add file to videoSplit for further processing
                    int x = 0;
                    for (int i = 0; i < 10; i++)
                    {
                        if (strcmp(v[i].filename, fname) == 0)
                        {
                            v[i].max++;
                            x = 1;
                            break;
                        }
                    }
                    if (x == 0)
                    {
                        sprintf(v[c].filename, "%s", fname);
                        v[c].max = 0;
                        c++;
                    }
                    break;
                }
                else
                {
                    p = strtok(NULL, ".");
                }
            }
        }
        closedir(dp);

        for (int i = 0; i < 10; i++)
        {
            if (strcmp(v[i].filename, "") == 0)
            {
                continue;
            }
            else
            {
                char fname[1000];
                sprintf(fname, "%s/%s/%s", dirpath, rot("Videos", 1), rot(v[i].filename, 1));
                printf("Merging file %s...\n", fname);
                FILE *p = fopen(fname, "w");
                if (p != NULL)
                {
                    for (int j = 0; j <= v[i].max; j++)
                    {
                        char benc[500];
                        sprintf(benc, "%s.%03d", v[i].filename, j);
                        char *enc = rot(benc, 1);
                        char sname[1000];
                        sprintf(sname, "%s/%s", dirpath, enc);
                        printf("\tfrom file %s...\n", sname);
                        FILE *q = fopen(sname, "r");
                        if (q != NULL)
                        {
                            do
                            {
                                int c = fgetc(q);
                                if (feof(q))
                                    break;
                                fputc(c, p);
                            } while (1);
                            fclose(q);
                        }
                        else
                        {
                            break;
                        }
                    }
                    fclose(p);
                }
                else
                {
                    printf("GAGAL\n");
                }
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t videoT;
    pthread_create(&videoT, NULL, &videoThread, NULL);

    umask(0);
    int a = fuse_main(argc, argv, &xmp_oper, NULL);

    pthread_join(videoT, NULL);
    removeVideos();
    return a;
}