-- beginning of machine-generated header
-- This is an auto-generated stub, it will be pre-pended by THiNX on cloud build.

majorVer, minorVer, devVer, chipid, flashid, flashsize, flashmode, flashspeed = node.info()

-- build-time constants
THINX_COMMIT_ID = "869fae3fc7d3a1f148e02bfa3da482f4d0ccfc4a"
THINX_FIRMWARE_VERSION_SHORT = majorVer.."."..minorVer.."."..devVer
THINX_FIRMWARE_VERSION = "nodemcu-esp8266-lua-"..THINX_FIRMWARE_VERSION_SHORT
THINX_UDID = node.chipid() -- each build is specific only for given udid to prevent data leak


-- dynamic variables (adjustable by user but overridden from API)
THINX_CLOUD_URL="thinx.cloud" -- can change to proxy (?)
THINX_MQTT_URL="thinx.cloud" -- should try thinx.local first for proxy
THINX_API_KEY="f40825e2de281390a6d8708d863cd431d573d417" -- will change in future to support rolling api-keys
THINX_DEVICE_ALIAS="nodemcu-lua-test"
THINX_DEVICE_OWNER="qool"
THINX_AUTO_UPDATE=true

THINX_MQTT_PORT = 1883
THINX_API_PORT = 7442

THINX_PROXY = "thinx.local"

-- end of machine-generated code

-- BEGINNING OF USER FILE

wifi_ssid='ssid'
wifi_password='password'
