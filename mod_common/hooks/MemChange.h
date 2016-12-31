#pragma once

#include <cstdint>
#include <string>
#include "Revertable.h"

class MemChange : public Revertable
{
    public:
        MemChange(uint32_t Addr): MemChange(reinterpret_cast<void*>(Addr)) {}
        MemChange(void *Addr);

		~MemChange()
		{
			Revert();
		}
        
        template<typename T>
        void Write(T Data)
        {
            Write(&Data, sizeof(Data));
        }
        
        void Write(const void *pData, size_t Len);
        virtual void Revert();
        
        void *GetAddr() const
        {
            return m_Addr;
        }

		const void *GetPrevData() const
		{
			return m_Backup.data();
		}
        
    protected:
        void *m_Addr;
        std::string m_Current;
        std::string m_Backup;
};
