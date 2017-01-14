#include "precomp.h"
#include "MemChange.h"
#include "log/Logger.h"

static void WriteProtectedMemory(void *Dest, const void *Src, size_t Len)
{
    DWORD dwOldProtect;
    if (VirtualProtect(Dest, Len, PAGE_EXECUTE_READWRITE, &dwOldProtect) == 0)
        logging::Logger::root().err() << "VirtualProtect failed";
    else
    {
        memcpy(Dest, Src, Len);
        VirtualProtect(Dest, Len, dwOldProtect, NULL);
		FlushInstructionCache(GetCurrentProcess(), Dest, Len);
    }
}

MemChange::MemChange(void *Addr):
    m_Addr(Addr) {}

void MemChange::Write(const void *pData, size_t Len)
{
	Revert();
    m_Backup.assign(reinterpret_cast<const char*>(m_Addr), Len);
    m_Current.assign(reinterpret_cast<const char*>(pData), Len);
    WriteProtectedMemory(m_Addr, pData, Len);
}

void MemChange::Revert()
{
    if (!m_Backup.empty())
    {
        if (memcmp(reinterpret_cast<const char*>(m_Addr), m_Current.data(), m_Current.size()) != 0)
            logging::Logger::root().warn() << "Changes not expected when reverting memory at " << std::hex << m_Addr;
        WriteProtectedMemory(m_Addr, m_Backup.data(), m_Backup.size());
        m_Backup.clear();
    }
}
