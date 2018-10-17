#include <stdio.h>


typedef struct dirorfile
{
    char *ContentServerAddress;
    int ContentServerPort;
    char *dirorfile_name;
    int delay;
}dirorfile;

typedef struct XBUFFER
{
    char *dirorfilename;
    char *ContentServerAddress;
    int ContentServerPort;

}XBUFFER;

typedef struct info
{
    char *dirname;
}info;
