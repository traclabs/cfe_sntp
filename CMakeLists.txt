project(CFE_SNTP C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src/coreSNTP/source/include)
include_directories(fsw/src)

# Create the app module
add_cfe_app(sntp fsw/src/sntp.c fsw/src/sntp_utils.c fsw/src/coreSNTP/source/core_sntp_serializer.c )

option(sntp_use_cfe_time "Use CFE time for SNTP. If disabled, use system time." ON)
if (sntp_use_cfe_time)
  target_compile_definitions(sntp PUBLIC -DSNTP_USE_CFE_TIME)
endif()

# Declare target include directories (exported to other tarets)
target_include_directories(ros_app PUBLIC
    fsw/mission_inc
    fsw/platform_inc
    )


# Include the public API from sample_lib to demonstrate how
# to call library-provided functions
#add_cfe_app_dependency(sntp sample_lib)

# Add table
#add_cfe_tables(sampleAppTable fsw/tables/sntp_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make 
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)

# Tools folder defines standalone SNTP Client and Server executables to test independent of cFE
# NOTE: tools directory must be compiled manually. cFE cmake configuration causes it to fail if invoked automatically
#add_subdirectory(tools)
