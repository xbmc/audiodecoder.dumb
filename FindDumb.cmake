# - Try to find dumb
# Once done this will define
#
# DUMB_FOUND - system has libdumb
# DUMB_INCLUDE_DIRS - the libdumb include directory
# DUMB_LIBRARIES - The libdumb libraries

find_path(DUMB_INCLUDE_DIRS dumb.h)
find_library(DUMB_LIBRARIES NAMES dumb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Dumb DEFAULT_MSG DUMB_INCLUDE_DIRS DUMB_LIBRARIES)

mark_as_advanced(DUMB_INCLUDE_DIRS DUMB_LIBRARIES)
