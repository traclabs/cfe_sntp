include_directories(../fsw/src/coreSNTP/source/include)
include_directories(../fsw/src)

# Create Native Test Apps
# Add executable for sntp_test_client
add_executable(sntp_test_client
  client_test.c
  ../fsw/src/coreSNTP/source/core_sntp_serializer.c
  ../fsw/src/sntp_utils.c
)


# Add executable for sntp_test_server
add_executable(sntp_test_server
  server_test.c
    ../fsw/src/coreSNTP/source/core_sntp_serializer.c
    ../fsw/src/sntp_utils.c
)

