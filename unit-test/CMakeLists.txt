##################################################################
#
# Coverage Unit Test build recipe
#
# This CMake file contains the recipe for building the sample unit tests.
# It is invoked from the parent directory when unit tests are enabled.
#
##################################################################

#
#
# NOTE on the subdirectory structures here:
#
# - "inc" provides local header files shared between the coveragetest,
#    wrappers, and overrides source code units
# - "coveragetest" contains source code for the actual unit test cases
#    The primary objective is to get line/path coverage on the FSW 
#    code units.
#
 
# Use the UT assert public API, and allow direct
# inclusion of source files that are normally private
include_directories(${PROJECT_SOURCE_DIR}/fsw/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)


# Add a coverage test executable called "sntp-ALL" that 
# covers all of the functions in sntp.  
#
# Also note in a more complex app/lib the coverage test can also
# be broken down into smaller units (in which case one should use
# a unique suffix other than "ALL" for each unit).  For example,
# OSAL implements a separate coverage test per source unit.
add_cfe_coverage_test(sntp ALL 
    "coveragetest/coveragetest_sntp.c"
    "${CFE_SNTP_SOURCE_DIR}/fsw/src/sntp.c"
)

# The sntp uses library functions provided by sample_lib so must be linked
# with the sample_lib stub library (this is mainly just an example of how this 
# can be done).
add_cfe_coverage_dependency(sntp ALL sample_lib)

