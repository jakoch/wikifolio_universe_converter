#
# xlsxio
#

find_library(xlsxio_read_LIBRARY
    NAMES xlsxio_read libxlsxio_read
    HINTS "${CMAKE_PREFIX_PATH}/lib"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(xlsxio_read DEFAULT_MSG xlsxio_read_LIBRARY xlsxio_read_INCLUDE_DIR)

mark_as_advanced(xlsxio_read_INCLUDE_DIR xlsxio_read_LIBRARY)

set(xlsxio_read_LIBRARIES    ${xlsxio_read_LIBRARY})
set(xlsxio_read_INCLUDE_DIRS ${xlsxio_read_INCLUDE_DIR})