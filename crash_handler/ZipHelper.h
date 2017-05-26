#pragma once

#include "zip.h"

class ZipHelper
{
private:
    zipFile m_zf;

public:
    ZipHelper(const char *path)
    {
        m_zf = zipOpen(path, 0);
    }

    ~ZipHelper()
    {
        zipClose(m_zf, NULL);
    }

    bool addFile(const char *path, const char *nameinZip)
    {
        zip_fileinfo zfi;
        memset(&zfi, 0, sizeof(zfi));
        getFileTime(path, &zfi.tmz_date, &zfi.dosDate);

        int err = zipOpenNewFileInZip(m_zf, nameinZip, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
        if (err != ZIP_OK)
            return false;

        FILE *srcFile = fopen(path, "rb");
        if (srcFile)
        {
            char buf[2048];
            while (!feof(srcFile))
            {
                size_t num = fread(buf, 1, sizeof(buf), srcFile);
                err = zipWriteInFileInZip(m_zf, buf, num);
                if (err != ZIP_OK)
                    return false;
            }
            fclose(srcFile);
        }

        zipCloseFileInZip(m_zf);
        return true;
    }

private:
    // getFileTime function based on minizip source
#ifdef WIN32
    uLong getFileTime(const char *filename, tm_zip *tmzip, uLong *dt)
    {
        int ret = 0;
        {
            FILETIME ftLocal;
            HANDLE hFind;
            WIN32_FIND_DATAA ff32;

            hFind = FindFirstFileA(filename, &ff32);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
                FileTimeToDosDateTime(&ftLocal, ((LPWORD)dt) + 1, ((LPWORD)dt) + 0);
                FindClose(hFind);
                ret = 1;
            }
        }
        return ret;
    }
#else
    uLong getFileTime(const char *filename, tm_zip *tmzip, uLong *dt)
    {
        int ret = 0;
        struct stat s;        /* results of stat() */
        struct tm* filedate;
        time_t tm_t = 0;

        if (stat(filename, &s) == 0)
        {
            tm_t = s.st_mtime;
            ret = 1;
        }
        filedate = localtime(&tm_t);

        tmzip->tm_sec = filedate->tm_sec;
        tmzip->tm_min = filedate->tm_min;
        tmzip->tm_hour = filedate->tm_hour;
        tmzip->tm_mday = filedate->tm_mday;
        tmzip->tm_mon = filedate->tm_mon;
        tmzip->tm_year = filedate->tm_year;

        return ret;
    }
#endif
};
