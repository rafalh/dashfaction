#pragma once

namespace rf
{
    class File
    {
        using SelfType = File;

        char m_internal_data[0x114];

    public:
        enum SeekOrigin {
            seek_set = 0,
            seek_cur = 1,
            seek_end = 2,
        };

        File()
        {
            AddrCaller{0x00523940}.this_call(this);
        }

        ~File()
        {
            AddrCaller{0x00523960}.this_call(this);
        }

        int Open(const char* filename, int flags = 1, int unk = 9999999)
        {
            return AddrCaller{0x00524190}.this_call<int>(this, filename, flags, unk);
        }

        void Close()
        {
            AddrCaller{0x005242A0}.this_call(this);
        }

        bool CheckVersion(int min_ver) const
        {
            return AddrCaller{0x00523990}.this_call<bool>(this, min_ver);
        }

        int GetError() const
        {
            return AddrCaller{0x00524530}.this_call<bool>(this);
        }

        int Seek(int pos, SeekOrigin origin)
        {
            return AddrCaller{0x00524400}.this_call<int>(this, pos, origin);
        }

        int Tell() const
        {
            return AddrCaller{0x005244E0}.this_call<int>(this);
        }

        int GetSize(const char *filename = nullptr, int a3 = 9999999) const
        {
            return AddrCaller{0x00524370}.this_call<int>(this, filename, a3);
        }

        int Read(void *buf, int buf_len, int min_ver = 0, int unused = 0)
        {
            return AddrCaller{0x0052CF60}.this_call<int>(this, buf, buf_len, min_ver, unused);
        }

        template<typename T>
        T Read(int min_ver = 0, T def_val = 0)
        {
            if (CheckVersion(min_ver)) {
                T val;
                Read(&val, sizeof(val));
                if (!GetError()) {
                    return val;
                }
            }
            return def_val;
        }
    };

    static auto& FileGetExt = AddrAsRef<char*(const char *path)>(0x005143F0);
    static auto& FileAddDirectoryEx = AddrAsRef<int(const char *dir, const char *ext_list, bool unknown)>(0x00514070);
    static auto& FileExists = AddrAsRef<bool(const char *filename)>(0x00544680);
    static auto& PackfileLoad = AddrAsRef<int(const char *file_name, const char *dir)>(0x0052C070);
    static auto& PackfileSetLoadingUserMaps = AddrAsRef<void(bool loading_user_maps)>(0x0052BB50);

    static auto& root_path = AddrAsRef<char[256]>(0x018060E8);
}
