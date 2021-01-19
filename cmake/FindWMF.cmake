# FindWMF
# ---------
#
# Try to locate the Windows Media Foundation library.
# If found, this will define the following variables:
#
# ``WMF_FOUND``
#     True if Windows Media Foundation is available
# ``WMF_LIBRARIES``
#     The Windows Media Foundation set of libraries
#
# If ``WMF_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``WMF::WMF``
#     The Windows Media Foundation library to link to

find_library(WMF_STMIIDS_LIBRARY strmiids)
find_library(WMF_DMOGUIDS_LIBRARY dmoguids)
find_library(WMF_UUID_LIBRARY uuid)
find_library(WMF_MSDMO_LIBRARY msdmo)
find_library(WMF_OLE32_LIBRARY ole32)
find_library(WMF_OLEAUT32_LIBRARY oleaut32)
find_library(WMF_MF_LIBRARY Mf)
find_library(WMF_MFUUID_LIBRARY Mfuuid)
find_library(WMF_MFPLAT_LIBRARY Mfplat)
find_library(WMF_PROPSYS_LIBRARY Propsys)

set(WMF_LIBRARIES ${WMF_STMIIDS_LIBRARY} ${WMF_DMOGUIDS_LIBRARY} ${WMF_UUID_LIBRARY} ${WMF_MSDMO_LIBRARY}
                ${WMF_OLE32_LIBRARY} ${WMF_OLEAUT32_LIBRARY} ${WMF_MF_LIBRARY}
                ${WMF_MFUUID_LIBRARY} ${WMF_MFPLAT_LIBRARY} ${WMF_PROPSYS_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WMF REQUIRED_VARS
                                WMF_STMIIDS_LIBRARY WMF_DMOGUIDS_LIBRARY WMF_UUID_LIBRARY WMF_MSDMO_LIBRARY
                                WMF_OLE32_LIBRARY WMF_OLEAUT32_LIBRARY WMF_MF_LIBRARY
                                WMF_MFUUID_LIBRARY WMF_MFPLAT_LIBRARY WMF_PROPSYS_LIBRARY)

if(WMF_FOUND AND NOT TARGET WMF::WMF)
    add_library(WMF::WMF INTERFACE IMPORTED)
    set_target_properties(WMF::WMF PROPERTIES
                          INTERFACE_LINK_LIBRARIES "${WMF_LIBRARIES}")
endif()

mark_as_advanced(WMF_LIBRARIES)
