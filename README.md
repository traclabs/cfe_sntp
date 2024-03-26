# SNTP Application

This application hosts a SNTP server inside of cFE that responds to requests at the standard UDP port (123).  The cFE-based server defaults to providing CFE TIME in UTC format, which may differ from the native system clock time.

It is based on the FreeRTOS coreSNTP library, which has been extended to provide limited server support.


The `tools` directory contains standalone client and server implementations for testing. Execute with '-h' for usage information, or run without arguments to use default settings. These can be built independently using cmake;

```
mkdir tools/build
cd tools/build
cmake ..
make
```

## Example Client Tests

The following commands query a [S]NTP server without changing the system time. They can be run from any system capable of reaching the cFE system at the standart port 123.  Wireshark can be used to troubleshoot any network connectivity issues.

- sntp -d localhost
  - Can be installed on debian-based systems with `sudo apt install sntp`
  - This client typically results in 3 NTP queries to the server.
- ./sntp_test_client
  - This tool can be built in the 'tools' directory using the instructions above and connects to localhost by default. Run with '-h' for additional options.  
  - A corresponding test server is also available to test functionality of this client without cfe.
  
