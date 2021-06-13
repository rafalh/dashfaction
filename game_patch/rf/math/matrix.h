#pragma once

#include "vector.h"
#include <patch_common/MemUtils.h>

namespace rf
{
    struct Matrix3
    {
        Vector3 rvec;
        Vector3 uvec;
        Vector3 fvec;

        void make_identity()
        {
            AddrCaller{0x004FCE70}.this_call(this);
        }

        Matrix3 copy_transpose() const
        {
            Matrix3 result;
            AddrCaller{0x004FC8A0}.this_call(this, &result);
            return result;
        }
    };
    static_assert(sizeof(Matrix3) == 0x24);

    struct Matrix43
    {
        Matrix3 orient;
        Vector3 origin;

        Matrix43 operator*(const Matrix43& other) const
        {
            Matrix43 result;
            AddrCaller{0x0051C620}.this_call(this, &result, &other);
            return result;
        }
    };

    static auto& identity_matrix = addr_as_ref<Matrix3>(0x0173C388);
}
