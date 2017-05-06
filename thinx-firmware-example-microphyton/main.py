# --- DEVELOPER NOTES ---

# Let's try it simplier without any Thinx.h in this case:
# In case of Micropython and LUA-based firmwares, there's always main file to
# be executed. Let's write our static variables to beginning of that file.

# Note: platformio uses .py files as custom scripts so we cannot simply match for .py to decide the project is micropython-based!
# We'll default to main.my then.

################################################################################
# --- beginning of machine-generated header

#
# This is an auto-generated file, it will be re-written by THiNX on cloud build.
#

# build-time constants
THINX_COMMIT_ID='869fae3fc7d3a1f148e02bfa3da482f4d0ccfc4a'
THINX_MQTT_URL='thinx.cloud' # should try thinx.local first for proxy
THINX_FIRMWARE_VERSION_SHORT = "0.0.1";
THINX_FIRMWARE_VERSION = "thinx-firmware-esp8266-py-0.0.1:2017-05-06"; # inject THINX_FIRMWARE_VERSION_SHORT
THINX_UDID='' # each build is specific only for given udid to prevent data leak

# dynamic variables (adjustable by user but overridden from API)
THINX_CLOUD_URL='thinx.cloud' # can change to proxy (?)
THINX_API_KEY='d0759aed29dcf1e9a1895e327a9e280039ce5784' # will change in future to support rolling api-keys
THINX_DEVICE_ALIAS='hardwired'
THINX_DEVICE_OWNER='test'
THINX_AUTO_UPDATE=true

THINX_MQTT_PORT = 1883
THINX_API_PORT = 7442

# --- end of machine-generated code
################################################################################

# --- BEGINNING OF USER FILE ---

################################################################################
# THiNX Example device application
################################################################################

# TODO: THX-69: Pure registration request.

# Roadmap:
# TODO: Perform update request and flash firmware over-the-air
# TODO: Support MQTT
# TODO: HTTPS proxy support
# TODO: MDNS Resolver

# Required parameters
THINX_API_KEY='ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890' # optional, this is just an override
SSID = 'SSID'
PASSWORD = 'PASSWORD'
TIMEOUT = 30

# Required for example - network connection
def do_connect():
    import network
    sta_if = network.WLAN(network.STA_IF)
    ap_if = network.WLAN(network.AP_IF)
    if ap_if.active():
        ap_if.active(False)
    if not sta_if.isconnected():
        print('THiNX: Connecting to WiFi...')
        sta_if.active(True)
        sta_if.connect(SSID, PASSWORD)
        while not sta_if.isconnected():
            pass
    print('THiNX: Network configuration:', sta_if.ifconfig())

# Example step 1: registration (device check-in)
def do_register():
    import urequests
    url = 'http://thinx.cloud:7442/device/register' # register/check-in device
    headers = {'Authentication': THINX_API_KEY,
               'Accept': 'application/json',
               'Origin': 'device',
               'Content-Type': 'application/json',
               'User-Agent': 'THiNX-Client'}
    data = '{"registration": {"mac": __DEVICE_MAC__, "firmware": THINX_FIRMWARE_VERSION, "version": THINX_FIRMWARE_VERSION_SHORT, "hash": "", "alias": "", "udid" : __DEVICE_MAC__ }}'
    resp = urequests.post(url, data=data, headers=headers)
    print(resp.json()) # return resp.json()['state']
    process_thinx_response(resp.json())

# Example step 2: device update
# TODO: Perform update request and flash firmware over-the-air
def fetch_update():
    url = 'http://thinx.cloud:7442/device/firmware' # firmware update retrieval by device mac
    headers = {'Authentication': THINX_API_KEY,
               'Accept': 'application/json',
               'Origin': 'device',
               'Content-Type': 'application/json',
               'User-Agent': 'THiNX-Client'}
               data = '{"registration": {"mac": __DEVICE_MAC__, "firmware": THINX_FIRMWARE_VERSION, "version": THINX_FIRMWARE_VERSION_SHORT, "hash": "", "alias": "", "udid" : __DEVICE_MAC__ }}'

    resp = urequests.post(url, data=data, headers=headers)
    print(resp.json()) # return resp.json()['state']
    process_thinx_response(resp.json())

# TODO: process incoming JSON response (both MQTT/HTTP) for register/force-update/update and forward others to client app)
def process_thinx_response(response):
    print("THiNX: response:")
    print(response)
    # if response['registration']
    reg = response['registration']
    if reg:
        # TODO: update device_info (global object in future, now just THINX_DEVICE_ALIAS, THINX_DEVICE_OWNER, THINX_API_KEY and THINX_UDID)
        THINX_DEVICE_ALIAS = reg['alias']
        THINX_DEVICE_OWNER = reg['owner']
        THINX_API_KEY = reg['apikey']
        THINX_UDID = reg['device_id']
        # TODO: save device info
        save_device_info(json_device_info())

    upd = response['update']
    if upd:
        if fetch_update():
            if THINX_AUTO_UPDATE:
                # TODO: Update firmware with something like:
                # https://github.com/pfalcon/yaota8266 - ota_server: step 4 only...

def json_device_info():
    print("THiNX: building device info:")
    # todo: should be dynamic, therefore all thinx info must be stored in serializable object
    json_object = {'alias': THINX_DEVICE_ALIAS,
                   'owner': THINX_DEVICE_OWNER,
                   'apikey': THINX_API_KEY,
                   'udid': THINX_UDID
                   }
    return json_object.json()

# TODO: Use this in response parser
def save_device_info(new_info):
    if f = open('thinx.cfg', 'w'):
        f.write(new_info)
        f.close()
    else:
        print("THINX: failed to open config file for writing")

# Restores incoming data from filesystem overriding build-time-constants
def restore_device_info():
    if f = open('thinx.cfg', 'r'): # can fail!
        info = f.read('some data')
        f.close()
        # override current values with incoming/restored values
        THINX_DEVICE_ALIAS=info['alias']
        THINX_DEVICE_OWNER=info['owner']
        THINX_API_KEY=['apikey']
        THINX_UDID=['device_id']
    else:
        print("THINX: No config file found")

# TODO: start MDNS resolver and attach to proxy if found
def do_mdns():
    # not in basic micropython firmware
    print("THINX: MDNS: To be implemented for proxy support")

# TODO: connect to mqtt and listen for updates or messages
def do_mqtt():
    # not in basic micropython firmware
    print("THINX: MQTT: To be implemented later")

def process_mqtt(response):
    # not in basic micropython firmware
    print("THINX: MQTT: To be implemented after that")

def main():
    import time

    restore_device_info()

    do_connect()
    do_register() # should start update automatically

    while True:
        try:
            do_register()
        except TypeError:
            pass
        time.sleep(TIMEOUT)

if __name__ == '__main__':
    print('THiNX: Register device.')
    main()
