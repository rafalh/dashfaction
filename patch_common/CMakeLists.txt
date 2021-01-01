set(SRCS
    CallHook.cpp
    CodeBuffer.cpp
    CodeInjection.cpp
    FunHook.cpp
    MemUtils.cpp
    include/patch_common/AsmOpcodes.h
    include/patch_common/AsmWriter.h
    include/patch_common/CallHook.h
    include/patch_common/CallPrePostHook.h
    include/patch_common/CodeBuffer.h
    include/patch_common/CodeInjection.h
    include/patch_common/ComPtr.h
    include/patch_common/FunHook.h
    include/patch_common/FunPrePostHook.h
    include/patch_common/InlineAsm.h
    include/patch_common/Installable.h
    include/patch_common/MemUtils.h
    include/patch_common/ShortTypes.h
    include/patch_common/StaticBufferResizePatch.h
    include/patch_common/Traits.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(PatchCommon STATIC ${SRCS})
target_compile_features(PatchCommon PUBLIC cxx_std_17)
set_target_properties(PatchCommon PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(PatchCommon)

target_compile_definitions(PatchCommon PUBLIC SUBHOOK_STATIC)

target_include_directories(PatchCommon PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/vendor/subhook
    ${CMAKE_SOURCE_DIR}/logger/include
)

target_link_libraries(PatchCommon subhook Xlog)
