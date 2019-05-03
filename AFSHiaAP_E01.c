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
#include <sys/wait.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

const char *dirpath = "/home/rifqi/shift4";
const char *cipher = "qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";

struct videoSplit
{
    char filename[256];
    int max;
};

char *fmttime(char *format)
{
    time_t timer;
    char *buffer = calloc(26, sizeof(char));
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, format, tm_info);
    return buffer;
}

const char *get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}

int direxists(const char *p)
{
    DIR *dir = opendir(p);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
        return 1;
    }
    else if (ENOENT == errno)
    {
        /* Directory does not exist. */
        return 0;
    }
    else
    {
        /* opendir() failed for some other reason. */
        return -1;
    }
}

int copyfile(const char *s, const char *d)
{
    char ch;
    FILE *source, *target;
    source = fopen(s, "r");
    if (source == NULL)
        return -1;
    target = fopen(d, "w");
    if (target == NULL)
    {
        fclose(source);
        return -1;
    }

    while ((ch = fgetc(source)) != EOF)
        fputc(ch, target);

    fclose(source);
    fclose(target);

    return 0;
}

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
                    strtorotate[i] = cipher[(j + 31) % strlen(cipher)];
                }
                else
                {
                    int r = (j - 31);
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

            struct stat info;
            memset(&info, 0, sizeof(info));
            stat(rot(de->d_name, 0), &info);
            struct passwd *pw = getpwuid(info.st_uid);
            struct group *gr = getgrgid(info.st_gid);
            time_t file_tm = info.st_atime;
            struct tm *atime = localtime(&file_tm);
            char strtime[100];
            strftime(strtime, 100, "%c", atime);

            char a[256];
            sprintf(a, "%s/%s", dirpath, rot("filemiris.txt",1));
            printf("%s\n",a);
            FILE *bahaya;
            bahaya = fopen(a, "a+");
            if (strcmp(pw->pw_name, "pristiz") == 0 || strcmp(pw->pw_name, "ic_controller") == 0 || strcmp(gr->gr_name, "rusak"))
            {
//                mode_t file_md = info.st_mode;
//                if ((file_md & S_IRWXU) && (file_md & S_IRWXG) && (file_md & S_IRWXO))
//                {
                    fprintf(bahaya, "%s\t%d\t%d\t%s\n", rot(de->d_name, 0), pw->pw_uid, gr->gr_gid, strtime);
//                }
            }

            fclose(bahaya);
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
    // int res;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    char backupdir[1000];
    sprintf(backupdir, "%s/%s", dirpath, rot("Backup", 1));

    char o[256];
    sprintf(o, "%s", path);
    char *p = basename(o);                 // b.cde
    const char *ext = get_filename_ext(p); // cde
    char q[256];                           // b.
    strncpy(q, p, strlen(p) - strlen(ext));

    //Scandir inside Backup, and rename all files
    struct dirent **namelist;
    int n;

    n = scandir(backupdir, &namelist, NULL, alphasort);
    if (n < 0)
        perror("scandir");
    else
    {
        chdir(backupdir);
        while (n--)
        {
            char k[1000];
            sprintf(k, "%s", rot(namelist[n]->d_name, 0));

            if (strstart(k, q) && strend(k, ext))
            {
                printf("Renaming from %s to %s\n", namelist[n]->d_name, k);
                rename(namelist[n]->d_name, k);
            }
            free(namelist[n]);
        }
        free(namelist);
    }

    //zip -m -j /RecycleBin/fileout.tmp.zip fileout.*.tmp /path/to/file/fileout.tmp
    char t[1000];
    sprintf(t, "%s/%s/%s%s", dirpath, rot("RecycleBin", 1), rot(p, 1), rot(".zip", 1));
    char u[1000];
    sprintf(u, "%s*.%s", q, ext);
    char y[1000];
    sprintf(y, "%s%s", dirpath, path);
    char t2[1000];
    sprintf(t2, "%s/%s/%s%s.zip", dirpath, rot("RecycleBin", 1), rot(p, 1), rot(".zip", 1));

    //renaming old file to new file
    printf("Renaming from %s to %s\n", a, y);
    rename(a, y);

    char *argvexec[7] = {"zip", "-m", "-j", "-o", t, u, y};

    pid_t child = fork();
    if (child != 0)
    {
        printf("Waiting for Zipping...\n");
        //wait until child fisished zipping
        wait(NULL);
        printf("Done Zipping...\n");
        //Renaming
        rename(t2, t);
    }
    else
    {
        //zipping
        printf("/usr/bin/zip zip -m -j %s %s %s\n", t, u, y);
        printf("***@@@@%s\n", backupdir);
        chdir(backupdir);
        execv("/usr/bin/zip", argvexec);
    }

    //File deleted by zip -m args
    // res = unlink(a);
    // if (res == -1)
    //     return -errno;

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

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(from, 1));

    char b[256];
    sprintf(b, "%s%s", dirpath, rot(to, 1));

    res = rename(a, b);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    int res;

    char ytFolder[1000];
    sprintf(ytFolder, "%s/%s", dirpath, rot("YOUTUBER", 1));

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    if (strstart(path, "/YOUTUBER") && !S_ISDIR(mode))
    {
        char o[256];
        sprintf(o, "%s", path);
        char *p = basename(o);                 // b.cde
        const char *ext = get_filename_ext(p); // cde
        char q[256];                           // b.
        strncpy(q, p, strlen(p) - strlen(ext));

        if(strcmp(ext,"iz1") == 0){
           printf("@@@@@@@@@@TIDAK BOLEH DIRUBAH!!!!\n");
            pid_t fk;
            fk = fork();
            if (fk == 0)
            {
                char *args[4] = {"zenity", "--warning", "--text=File ekstensi iz1 tidak boleh diubah permissionnya.", NULL};
                execv("/usr/bin/zenity", args);
            }
            return -errno;
        }
    }

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

    if (strstart(path, "/YOUTUBER"))
        mode = 0640;

    (void)fi;

    res = creat(a, mode);
    if (res == -1)
        return -errno;
    close(res);

    if (strstart(path, "/YOUTUBER"))
    {
        char k[1000];
        sprintf(k, "%s/%s%s", dirpath, rot(path,1), rot(".iz1",1));

        rename(a,k);
    }

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

    char o[256];
    sprintf(o, "%s", path);

    // a/b.cde
    char *p = basename(o);                 // b.cde
    const char *ext = get_filename_ext(p); // cde
    char q[256];                           // b
    strncpy(q, p, strlen(p) - strlen(ext));
    char *t = fmttime("%Y-%m-%d_%H:%M:%S");
    char final[500]; //b_time.cde
    sprintf(final, "%s_%s.%s", q, t, ext);

    char backuppath[500];
    sprintf(backuppath, "%s/%s", dirpath, rot("Backup", 1));
    if (direxists(backuppath) < 1)
    {
        printf("##MAKING DIRECTORY BACKUP\n");
        mkdir(backuppath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    char a[1000];
    sprintf(a, "%s%s", dirpath, rot(path, 1));

    char b[1000];
    sprintf(b, "%s/%s", backuppath, rot(final, 1));

    copyfile(a, b);
    printf("##BACKING UP FROM %s to %s\n", a, b);

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

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

    char a[256];
    sprintf(a, "%s%s", dirpath, rot(path, 1));
	res = utimes(a, tv);
	if (res == -1)
		return -errno;

	return 0;
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
    .utimens = xmp_utimens,
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