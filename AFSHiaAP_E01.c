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

const char *dirpath = "/home/rifqi/shift4";
const char *cipher = "qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";

int strend(const char *s, const char *t)
{
    char lastl[strlen(t)];
    int delta = strlen(s) - strlen(t);
    sprintf(lastl, "%s", s + delta);
    return (strcmp(lastl, t) == 0 ? 1 : 0);
}

const char *rot(const char *a, int enc)
{
    if (strend(a, ".") == 1 || strend(a, "..") == 1)
        return a;
    char *strtorotate = malloc(strlen(a) * sizeof(char) + 1);
    sprintf(strtorotate,"%s",a);
    for (int i = 0; i < strlen(strtorotate); i++)
    {
        if(strtorotate[i] == '\0') break;
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
    char dname[1000];
    sprintf(dname, "%s%s", dirpath, rot(path, 1));
    if(strend(path,"/.")){
        dname[strlen(dname) - 2] = '.';
    }else if(strend(path,"/..")){
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
    char dname[1000];
    sprintf(dname, "%s%s", dirpath, rot(path, 1));
    if(strend(path,"/.")){
        dname[strlen(dname) - 2] = '.';
    }else if(strend(path,"/..")){
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
        if (filler(buf, rot(de->d_name,0), &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    int res;

    char a[1000];
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
    // .read		= xmp_read,
};

int main(int argc, char *argv[])
{
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}