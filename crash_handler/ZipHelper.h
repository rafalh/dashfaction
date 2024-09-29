#pragma once

#include "zip.h"

class ZipHelper
{
private:
    zipFile m_zf;

public:
    ZipHelper(const char* path, bool create = true)
    {
        int append = create ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP;
        m_zf = zipOpen(path, append);
        if (!m_zf)
            throw std::runtime_error("zipOpen failed");
    }

    ~ZipHelper()
    {
        if (m_zf) {
            try {
                close();
            }
            catch (...) {
                // Destructor cannot throw
            }
        }
    }

    void add_file(const char* path, const char* name_in_zip)
    {
        zip_fileinfo zfi{};
        get_file_time(path, &zfi.tmz_date, &zfi.dosDate);

        int err = zipOpenNewFileInZip(m_zf, name_in_zip, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
        if (err != ZIP_OK)
            throw std::runtime_error("zipOpenNewFileInZip failed");

        std::ifstream file(path, std::ios_base::in | std::ios_base::binary);
        if (!file)
            THROW_EXCEPTION("cannot open {}", path);

        char buf[4096];
        while (!file.eof()) {
            file.read(buf, sizeof(buf));
            size_t num = static_cast<size_t>(file.gcount());
            err = zipWriteInFileInZip(m_zf, buf, num);
            if (err != ZIP_OK)
                throw std::runtime_error("zipWriteInFileInZip failed");
        }

        zipCloseFileInZip(m_zf);
    }

    void add_file(const char* name_in_zip, std::string_view content)
    {
        zip_fileinfo zfi{};

        int err = zipOpenNewFileInZip(m_zf, name_in_zip, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
        if (err != ZIP_OK)
            throw std::runtime_error("zipOpenNewFileInZip failed");

        err = zipWriteInFileInZip(m_zf, content.data(), content.size());
        if (err != ZIP_OK)
            throw std::runtime_error("zipWriteInFileInZip failed");

        zipCloseFileInZip(m_zf);
    }

    void close()
    {
        int err = zipClose(m_zf, NULL);
        if (err != ZIP_OK)
            throw std::runtime_error("zipClose failed");
        m_zf = nullptr;
    }

private:
    // getFileTime function based on minizip source
#ifdef WIN32
    uLong get_file_time(const char* filename, [[maybe_unused]] tm_zip* tmzip, uLong* dt)
    {
        int ret = 0;
        FILETIME ftLocal;
        HANDLE hFind;
        WIN32_FIND_DATAA ff32;

        hFind = FindFirstFileA(filename, &ff32);
        if (hFind != INVALID_HANDLE_VALUE) {
            FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
            FileTimeToDosDateTime(&ftLocal, ((LPWORD)dt) + 1, ((LPWORD)dt) + 0);
            FindClose(hFind);
            ret = 1;
        }
        return ret;
    }
#else  // WIN32
    uLong get_file_time(const char* filename, tm_zip* tmzip, [[maybe_unused]] uLong* dt)
    {
        int ret = 0;
        struct stat s; /* results of stat() */
        struct tm* filedate;
        time_t tm_t = 0;

        if (stat(filename, &s) == 0) {
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
#endif // WIN32
};
