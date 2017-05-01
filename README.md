# thinx-esp8266-firmware

# Requirements

- Arduino IDE or Platform.io
- Arduino libraries: ArduinoJSON, WiFiManager, ESP8266httpUpdate (to be replaced)

# Work in progress!

WARNING! Open this folder using Atom with installed Platform.io or thinx-firmware-esp8266/thinx-firmware-esp8266.ino using Arduino IDE.

Contents of thinx-lib-esp is not working yet.

Run prerelease.sh to bake your commit ID into the Thinx.h file.

# Usage

1. Create API Key on the thinx.cloud site (you can store this in Thinx.h file in case your project is not stored in public repository).
2. Include the library and Thinx.h file into your project.
3. Build and upload the code to your device.
4. Wait for AP-THiNX WiFi SSID to appear
5. Connect with password 'PASSWORD'
6. Connect the device using its Captive portal to your WiFI; if you did not store your API Key in source for security reasons, you must enter it at least NOW
7. Device will connect to WiFi and register itself. Check your thinx.cloud dashboard for new device.

Note: In case you'll build/upload your project (e.g. the library) using thinx.cloud, API key will be injected automatically and you should not need to set it up anymore.

TODO: Prevent adding apikey parameter to Captive Portal when aready exists? Really? Ask users.
