# --- DEVELOPER NOTES ---

# In case of Micropython and LUA-based firmwares, there's always main file to
# be executed. Builder will write our static variables to beginning of that file.

# Note: platformio uses .py files as custom scripts so we cannot simply match for .py to decide the project is micropython-based!
# We'll default to main.my then.

################################################################################
# --- beginning of machine-generated header

#
# This is an auto-generated stub, it will be pre-pended by THiNX on cloud build.
#

import os

# build-time constants
THINX_COMMIT_ID="micropython"
THINX_FIRMWARE_VERSION_SHORT = os.uname().release.split("(")[0] # remove non-semantic part
THINX_FIRMWARE_VERSION = os.uname().version # inject THINX_FIRMWARE_VERSION_SHORT
THINX_UDID="" # each build is specific only for given udid to prevent data leak

# dynamic variables (adjustable by user but overridden from API)
THINX_CLOUD_URL="thinx.cloud" # can change to proxy (?)
THINX_MQTT_URL="thinx.cloud" # should try thinx.local first for proxy
THINX_API_KEY="b63f960e3a05b513661ea8ee001e3a17e14228ba" # will change in future to support rolling api-keys
THINX_DEVICE_ALIAS="hardwired"
THINX_DEVICE_OWNER="test"
THINX_AUTO_UPDATE=True

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
# TODO: Perform update request and replace boot.py OVER-THE-AIR (may fail)
# TODO: Support MQTT
# TODO: HTTPS proxy support
# TODO: MDNS Resolver
# TODO: convert to thinx module

# Required parameters
SSID = '6RA'
PASSWORD = 'quarantine'
TIMEOUT = 180

import urequests
import ubinascii
import network
import time
import ujson

# Prerequisite: WiFi connection
def connect(ssid, password):
    sta_if = network.WLAN(network.STA_IF)
    ap_if = network.WLAN(network.AP_IF)
    if ap_if.active():
        ap_if.active(False)
    if not sta_if.isconnected():
        print('THiNX: Connecting to WiFi...')
        sta_if.active(True)
        sta_if.connect(ssid, password)
        while not sta_if.isconnected():
            pass
    else:
        THINX_UDID=sta_if.config('mac')
        thinx_register()
        thinx_mqtt()

    print('THiNX: Network configuration:', sta_if.ifconfig())

# Example step 1: registration (device check-in)
def thinx_register():
    print('THiNX: Device registration...')
    url = 'http://thinx.cloud:7442/device/register' # register/check-in device
    headers = {'Authentication': THINX_API_KEY,
               'Accept': 'application/json',
               'Origin': 'device',
               'Content-Type': 'application/json',
               'User-Agent': 'THiNX-Client'}
    print(headers)
    registration_request = { 'registration': {'mac': thinx_mac(),
                    'firmware': THINX_FIRMWARE_VERSION,
                    'version': THINX_FIRMWARE_VERSION_SHORT,
                    'hash': THINX_COMMIT_ID,
                    'alias': THINX_DEVICE_ALIAS,
                    'owner': THINX_DEVICE_OWNER,
                    'udid': THINX_UDID}}
    data = ujson.dumps(registration_request)
    print(data)
    resp = urequests.post(url, data=data, headers=headers)

    if resp:
        print("THiNX: Server replied...")
        print(resp.json())
        process_thinx_response(resp.json())
    else:
        print("THiNX: No response.")

    resp.close()

# Example step 2: device update
# TODO: Perform update request and flash firmware over-the-air
# API expects: mac, hash (?), checksum, commit, owner
def thinx_update(data):
    url = 'http://thinx.cloud:7442/device/firmware' # firmware update retrieval by device mac
    headers = {'Authentication': THINX_API_KEY,
               'Accept': 'application/json',
               'Origin': 'device',
               'Content-Type': 'application/json',
               'User-Agent': 'THiNX-Client'}
    print(headers)
    update_request = { 'update': {'mac': thinx_mac(),
                       'hash': data.hash,
                       'checksum': data.checksum,
                       'commit': data.commit,
                       'alias': data.alias,
                       'owner': data.owner,
                       'udid': THINX_UDID }}

    data = ujson.dumps(update_request)
    print(data)

    resp = urequests.post(url, data=data, headers=headers)

    if resp:
        print("THiNX: Server replied...")
        print(resp.json())
        process_thinx_response(resp.json())
    else:
        print("THiNX: No response.")

    resp.close()

def process_thinx_response(response):

    #print("THINX: Parsing response:")    
    #print(response)

    try:
        success = response['success']
        print("THINX: Parsing success response:")
        print(response)
        if success==False:
            print("THiNX: Failure.")
            return
    except Exception:
        print("THINX: No primary success key found.")
    
    try:
        reg = response['registration']
        print("THINX: Parsing registration response:")
        print(reg)
    except Exception:
        print("THiNX: No registration key found.")

    if reg:
        try:
            success = reg['success']
            if success==False:
                print("THiNX: Registration failure.")
                return                               
        except Exception:
                print("THINX: No registration success key...")            

        #THINX_DEVICE_ALIAS = reg['alias']
        #THINX_DEVICE_OWNER = reg['owner']
        #THINX_API_KEY = reg['apikey']
        
        try:
            THINX_UDID = reg['device_id']
            print("THINX: Overriding device_id: " + THINX_UDID)            
        except Exception:            
            pass
        
        save_device_info()     
               
    try:
        upd = response['update']
        if upd:
            if thinx_update():
                if THINX_AUTO_UPDATE:
                    print("TODO: Update firmware") # https://github.com/pfalcon/yaota8266 - ota_server: step 4 only...
    except Exception:
        print("THiNX: No update key found.")
    

    print("THiNX: Parser completed.")

# provides only current status as JSON so it can be loaded/saved independently
def get_device_info():
    json_object = {'alias': THINX_DEVICE_ALIAS,
                   'owner': THINX_DEVICE_OWNER,
                   'apikey': THINX_API_KEY,
                   'udid': THINX_UDID
                   }
    return ujson.dumps(json_object)

# apply given device info to current runtime environment
def set_device_info(info):
    THINX_DEVICE_ALIAS=info['alias']
    THINX_DEVICE_OWNER=info['owner']
    THINX_API_KEY=info['apikey']
    THINX_UDID=info['device_id']

# Used by response parser
def save_device_info():
    print("THINX: Saving device info")
    f = open('thinx.cfg', 'w')
    if f:
        f.write(get_device_info())
        f.close()
    else:
        print("THINX: failed to open config file for writing")

# Restores incoming data from filesystem overriding build-time-constants
def restore_device_info():
    try:
        f = open('thinx.cfg', 'r')
        if f:
            print("THINX: Restoring device info")
            info = f.read('\n')
            f.close()
            set_device_info(ujson.loads(info))
        else:
            print("THINX: No config file found")
    except Exception:
        print("THINX: No config file found...")

def thinx_mdns():
    print("TODO: MDNS resolver")
    # Basic firmware does not support resolver

def thinx_mqtt():
    # not in basic micropython firmware
    print("THINX: MQTT: To be implemented later")

def process_mqtt(response):
    print(response)
    process_thinx_response(response)

# local platform helpers

def thinx_mac():
    wlan = network.WLAN(network.STA_IF)
    mac = wlan.config('mac')
    return ubinascii.hexlify(mac) # todo: convert to string, returs binary!

# main library function

def thinx():
    global THINX_UDID

    restore_device_info()

    if THINX_UDID=="":
        THINX_UDID=thinx_mac() # until given by API

    connect(SSID, PASSWORD) # calls register and mqtt

# sample app

def main():

    while True:
        try:
            thinx()
        except TypeError:
            pass
        time.sleep(TIMEOUT)

if __name__ == '__main__':
    print('THiNX: Register device.')
    main()
