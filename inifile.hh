/********************************************************************
* Description: inifile.hh
*   Declarations for INI file format functions
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change: 
********************************************************************/

#ifndef INIFILE_HH
#define INIFILE_HH

#include <stdio.h>
#include <fcntl.h>


#ifdef __cplusplus
class IniFile {
public:
    typedef enum {
        ERR_NONE                = 0x00,
        ERR_NOT_OPEN            = 0x01,
        ERR_SECTION_NOT_FOUND   = 0x02,
        ERR_TAG_NOT_FOUND       = 0x04,
        ERR_CONVERSION          = 0x08,
        ERR_LIMITS              = 0x10,
    } ErrorCode;

    class Exception {
    public:
        ErrorCode               errCode;
        const char *            tag;
        const char *            section;
        int                     num;
        unsigned int            lineNo;

        void                    Print(FILE *fp=stderr);
    };


                                IniFile(int errMask=0, FILE *fp=NULL);
                                ~IniFile(void){ Close(); }

    bool                        Open(const char *file);
    bool                        Close(void);
    bool                        IsOpen(void){ return(fp != NULL); }
    ErrorCode                   Find(int *result, int min, int max,
                                     const char *tag,const char *section,
                                     int num=1);
    ErrorCode                   Find(int *result, const char *tag,
                                     const char *section=NULL, int num = 1);
    ErrorCode                   Find(double *result, double min, double max,
                                     const char *tag,const char *section,
                                     int num=1);
    ErrorCode                   Find(double *result, const char *tag,
                                     const char *section=NULL, int num = 1);
    const char *                Find(const char *tag, const char *section=NULL,
                                     int num = 1);
    void                        EnableExceptions(int _errMask){
                                    errMask = _errMask;
                                }


protected:
    struct StrIntPair {
        char                    *pStr;
        int                     value;
    };

    struct StrDoublePair {
        char                    *pStr;
        double                   value;
    };


    ErrorCode                   Find(double *result, StrDoublePair *,
                                     const char *tag, const char *section=NULL,
                                     int num = 1);
    ErrorCode                   Find(int *result, StrIntPair *,
                                     const char *tag, const char *section=NULL,
                                     int num = 1);


private:
    FILE                        *fp;
    struct flock                lock;
    bool                        owned;

    Exception                   exception;
    int                         errMask;

    unsigned int                lineNo;
    const char *                tag;
    const char *                section;
    int                         num;

    bool                        CheckIfOpen(void);
    bool                        LockFile(void);
    void                        TildeExpansion(const char *file, char *path);
    void                        ThrowException(ErrorCode);
    char                        *AfterEqual(const char *string);
    char                        *SkipWhite(const char *string);
};
#else
extern const char *iniFind(FILE *fp, const char *tag, const char *section);
#endif


#endif
