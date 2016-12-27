#include "rf.h"

namespace rf
{
    constexpr auto SetupPlayerWeaponMesh2 = (void(*)(CPlayer *pPlayer, int WeaponClsId))0x004AA230;
};


int test()
{
    rf::SetupPlayerWeaponMesh2(nullptr, 1);
    return 0;
}