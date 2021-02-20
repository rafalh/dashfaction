if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(VPP_TOOL ${CMAKE_SOURCE_DIR}/tools/vpp.exe)
else()
    set(VPP_TOOL ${CMAKE_SOURCE_DIR}/tools/vpp)
endif()

# Build dashfaction.vpp
# Note: only Ninja generator supports DEPFILE currently
if (CMAKE_GENERATOR STREQUAL "Ninja")
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/bin/dashfaction.vpp
        COMMAND ${VPP_TOOL} -c ${CMAKE_BINARY_DIR}/bin/dashfaction.vpp @dashfaction_vpp_filelist.txt --dep-info
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/dashfaction_vpp_filelist.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPFILE ${CMAKE_BINARY_DIR}/bin/dashfaction.vpp.d
    )
else()
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/bin/dashfaction.vpp
        COMMAND ${VPP_TOOL} -c ${CMAKE_BINARY_DIR}/bin/dashfaction.vpp @dashfaction_vpp_filelist.txt --dep-info
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/dashfaction_vpp_filelist.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
add_custom_target(DashFactionVpp ALL DEPENDS ${CMAKE_BINARY_DIR}/bin/dashfaction.vpp)

# Copy licensing-info.txt
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/licensing-info.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/licensing-info.txt ${CMAKE_BINARY_DIR}/bin/licensing-info.txt
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/licensing-info.txt
)
add_custom_target(LicensingInfoTxt ALL DEPENDS ${CMAKE_BINARY_DIR}/bin/licensing-info.txt)
