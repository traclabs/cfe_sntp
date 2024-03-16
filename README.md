# SNTP Application

This application hosts a SNTP server inside of cFE that responds to requests at the standard UDP port (123).  The cFE-based server defaults to providing CFE TIME in UTC format, which may differ from the native system clock time.

It is based on the FreeRTOS coreSNTP library, which has been extended to provide limited server support.


The `tools` directory contains standalone client and server implementations for testing. Execute with '-h' for usage information, or run without arguments to use default settings. These can be built independently using cmake;
- mkdir tools/build
- cd tools/build
- cmake ..
- make
