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
    };
    static_assert(sizeof(Matrix3) == 0x24);

    static auto& identity_matrix = addr_as_ref<Matrix3>(0x0173C388);
}
