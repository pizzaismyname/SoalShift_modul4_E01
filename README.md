# SoalShift_modul4_E01

### Soal 1
- Fungsi untuk mengubah nama file sesuai dengan _caesar cipher_ (_key_ yang dipakai 17).
```c
const char *cipher = "qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";

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
            //mencari karakter nama file pada character list yang diberikan.
            if (cipher[j] == strtorotate[i])
            {
                // untuk enkripsi
                if (enc == 1)
                {
                    strtorotate[i] = cipher[(j + 17) % strlen(cipher)];
                }
                //untuk dekripsi
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
```

### Soal 2
- Membuat folder "Videos" secara otomatis di root directory file system.
```c
void *videoThread()
{
    //...
    sprintf(d, "%s/%s", dirpath, rot("Videos", 1));
    mkdir(d, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //...
}
```
- Menggabungkan file-file pecahan
```c
void *videoThread()
{
    //...
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
    //...
}
```
### Soal 3

### Soal 4

### Soal 5