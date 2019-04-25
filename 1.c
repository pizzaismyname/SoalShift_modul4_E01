#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *cipher = "qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";
const char *dir = "/home/pristiz/Documents";

char *rot(const char *a, int enc)
{
    char *strtorotate = malloc(strlen(a) * sizeof(char));
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
    return strtorotate;
}

const char *translatePath(const char *path, int a)
{
    int s = strlen(dir);
    int t = strlen(path);

    char *l = malloc(sizeof(char) * s);
    memcpy(l, path, s);

    if (strcmp(l, dir) == 0)
    {
        char *b = malloc(sizeof(char) * (t - s));
        memcpy(b, path + s, t - s);
        char *c = malloc(sizeof(char) * t);
        memcpy(c, dir, s);
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

int strend(const char *s, const char *t)
{
    char lastl[strlen(t)];
    int delta = strlen(s) - strlen(t);
    sprintf(lastl, "%s", s + delta);
    return (strcmp(lastl, t) == 0 ? 1 : 0);
}

int strstart(const char *s, const char *t){
    char lastl[strlen(t) + 1];
    int delta = strlen(s) - strlen(t);
    memcpy(lastl,s,strlen(t));
    lastl[strlen(t)] = '\0';
    return (strcmp(lastl, t) == 0 ? 1 : 0);
}

int main()
{
    printf("%s/%s.%03d", dir, "movie.mkv", 0);
    return 0;
}