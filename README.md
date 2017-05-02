# thinx-esp8266-firmware

# Requirements

- Arduino IDE or Platform.io
- Arduino libraries: ArduinoJSON, WiFiManager, ESP8266httpUpdate (to be replaced)

# Work in progress!

### TODOs:

* THX-48: Upgrade ESP866HTTPUpdate to fetch firmware from unique generated URL (or with API Key)

### WARNING!

Open this folder using Atom with installed Platform.io or thinx-firmware-esp8266/thinx-firmware-esp8266.ino using Arduino IDE.

Contents of thinx-lib-esp is not working yet.

Run prerelease.sh to bake your commit ID into the Thinx.h file.

# Usage

1. Create account on the [http://rtm.thinx.cloud/](http://rtm.thinx.cloud/) site
2. Create an API Key
3. Clone [vanilla NodeMCU app repository](https://github.com/suculent/thinx-firmware-esp8266) 
4. You can store API Key in Thinx.h file in case your project is not stored in public repository.
5. Build and upload the code to your device.
6. After restart, connect with some device to WiFi AP 'AP-THiNX' with password 'PASSWORD' and enter the API Key
7. Device will connect to WiFi and register itself. Check your thinx.cloud dashboard for new device.

... Then you can theoretically add own git source, add ssh-keys to access those sources if not public, attach the source to device to dashboard and click the last icon in row to build/update the device. But that's not tested at time of updating this readme.


Note: In case you'll build/upload your project (e.g. the library) using thinx.cloud, API key will be injected automatically and you should not need to set it up anymore.

TODO: Prevent adding apikey parameter to Captive Portal when aready exists? Really? Ask users.
