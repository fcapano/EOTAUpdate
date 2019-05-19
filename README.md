# What is this

EOTAUpdate is a library for ESP32 that can periodically check for available updates, fetch and
flash them with minimal configuration.
- It works over secured and non-secured HTTP connections.
- It fetches a text file that states which is the latest version and where to find it
- By default it checks for updates every hour

# Setup

- Copy the file ota/cfg.txt to a server that can be reached by your ESP32 and update the fields as needed.

-  The file contains four lines, the URL to the firmware to install, a number representing its version, an optional MD5 of the binary
   and an optional description of the version.
   If not including optional field, leave an empty line in its place.

- Add the following lines to the beginning your main.cpp

      #include <EOTAUpdate.h>
      const unsigned VERSION_NUMBER = 1;
      const String   UPDATE_URL     = "https://myserver/ota/cfg.txt";
      EOTAUpdate updater(UPDATE_URL, VERSION_NUMBER);

- And in the loop() function call:

      updater.CheckAndUpdate();

# Release an update

When releasing an update remember to

- Update the constant VERSION_NUMBER in your code, otherwise you might end up with an update loop
- Upload the firmware to the sever
- Update the cfg.txt with the newly released version number and string
