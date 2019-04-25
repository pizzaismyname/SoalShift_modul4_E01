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

const char *dirpath = "/home/pristiz/shift4";

const char *cipher = "qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";

int strend(const char *s, const char *t)
{
    size_t ls = strlen(s); // find length of s
    size_t lt = strlen(t); // find length of t
    if (ls >= lt)          // check if t can fit in s
    {
        // point s to where t should start and compare the strings from there
        return (0 == memcmp(t, s + (ls - lt), lt));
    }
    return 0; // t was longer than s
}

const char *rot(const char *a, int enc)
{
    printf("ROT %d %s \n", enc,  a);
    if (strend(a, ".") == 1 || strend(a, "..") == 1)
        return a;
    char *strtorotate = malloc(strlen(a) * sizeof(char) + 1);
    memcpy(strtorotate, a, strlen(a));
    for (int i = 0; i < strlen(strtorotate); i++)
    {
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
    strcat(strtorotate, "\0");
    return strtorotate;
}

const char *translatePath(const char *path, int a)
{
    int s = strlen(dirpath);
    int t = strlen(path);

    char *l = malloc(sizeof(char) * s);
    memcpy(l, path, s);

    if (strcmp(l, dirpath) == 0)
    {
        char *b = malloc(sizeof(char) * (t - s));
        memcpy(b, path + s, t - s);
        char *c = malloc(sizeof(char) * t);
        memcpy(c, dirpath, s);
        memcpy(c + s, rot(b, a), t - s);
        free(b);
        free(l);
        return c;
    }
    else
    {
        free(l);
        return path;
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;

    char a[1000] = "";
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = lstat(a, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;

    (void)offset;
    (void)fi;

    char a[1000] = "";
    sprintf(a, "%s%s", dirpath, rot(path, 1));
    int res = 0;

    dp = opendir(a);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        res = (filler(buf, rot(de->d_name, 0), &st, 0));
        if (res != 0)
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    int res;

    char a[1000] = "";
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    res = mkdir(a, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .mkdir = xmp_mkdir,
    // .read = xmp_read,
};

int main(int argc, char *argv[])
{
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}