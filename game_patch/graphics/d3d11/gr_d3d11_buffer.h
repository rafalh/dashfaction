#pragma once

#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11.h"

namespace df::gr::d3d11
{
    template<typename T>
    class RingBuffer
    {
    public:
        RingBuffer(int buffer_size, UINT bind_flags, ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_context) :
            buffer_size_{buffer_size}, device_{device}, device_context_{device_context}
        {
            D3D11_BUFFER_DESC buffer_desc;
            ZeroMemory(&buffer_desc, sizeof(buffer_desc));
            buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
            buffer_desc.ByteWidth        = buffer_size * sizeof(T);
            buffer_desc.BindFlags        = bind_flags;
            buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;

            HRESULT hr = device_->CreateBuffer(&buffer_desc, nullptr, &buffer_);
            check_hr(hr, "CreateBuffer");
        }

        ~RingBuffer()
        {
            submit();
        }

        std::pair<T*, int> alloc(int size)
        {
            bool buffer_full = is_full(size);
            assert(!mapped_data_ || !buffer_full);
            if (!mapped_data_) {
                D3D11_MAP map_type = buffer_full ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
                D3D11_MAPPED_SUBRESOURCE mapped_subres;
                HRESULT hr = device_context_->Map(buffer_, 0, map_type, 0, &mapped_subres);
                check_hr(hr, "Map");
                mapped_data_ = reinterpret_cast<T*>(mapped_subres.pData);
                if (buffer_full) {
                    start_pos_ = current_pos_ = 0;
                }
            }

            T* allocated_data = mapped_data_ + current_pos_;
            int allocated_pos = current_pos_;
            current_pos_ += size;
            return {allocated_data, allocated_pos};
        }

        std::pair<int, int> submit()
        {
            if (mapped_data_) {
                device_context_->Unmap(buffer_, 0);
                mapped_data_ = nullptr;
            }
            std::pair<int, int> result{start_pos_, current_pos_ - start_pos_};
            start_pos_ = current_pos_;
            return result;
        }

        bool is_full(int size) const
        {
            return current_pos_ + size > buffer_size_;
        }

        ID3D11Buffer* get_buffer() const
        {
            return buffer_;
        }

    private:
        int buffer_size_;
        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> device_context_;
        ComPtr<ID3D11Buffer> buffer_;
        T* mapped_data_ = nullptr;
        int start_pos_ = 0;
        int current_pos_ = 0;
    };
}
