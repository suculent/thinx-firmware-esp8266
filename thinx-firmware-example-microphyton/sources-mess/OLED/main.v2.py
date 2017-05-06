import ssd1306
import utime
import network
import machine
import esp
import time
import ubinascii
import webrepl

import logo

from machine import I2C, Pin, Timer, ADC
from umqtt.simple import MQTTClient

print("Booting in 5 seconds...")
utime.sleep(5)

# MQTT

# These defaults are overwritten with the contents of /config.json by load_config()
CONFIG = {
    "broker": "192.168.1.21",
    "sensor_pin": 0, 
    "client_id": b"esp8266_" + ubinascii.hexlify(machine.unique_id()),
    "topic": b"/display/1",
}

client = None
sensor_pin = None

def setup_pins():
    global sensor_pin
    sensor_pin = machine.ADC(CONFIG['sensor_pin'])

def load_config():
    import ujson as json
    try:
        with open("/config.json") as f:
            config = json.loads(f.read())
    except (OSError, ValueError):
        print("Couldn't load /config.json")
        save_config()
    else:
        CONFIG.update(config)
        print("Loaded config from /config.json")

def save_config():
    import ujson as json
    try:
        with open("/config.json", "w") as f:
            f.write(json.dumps(CONFIG))
    except OSError:
        print("Couldn't save /config.json")

# ORIGINAL CODE

print('machine frequency: ', machine.freq()/1000000, ' MHz')
wlan = network.WLAN(network.STA_IF)

def do_ap():
  ap = network.WLAN(network.AP_IF)
  ap.active(True)
  ap.config(essid='ESP-AP')

def do_connect():        
    wlan.active(True)
    if not wlan.isconnected():
        print('connecting to network...')
        wlan.connect('page42.showflake.czf', 'quarantine')
        while not wlan.isconnected():
            pass
    print('network config:', wlan.ifconfig())

def draw_line(display, x0, y0, x1, y1):
    deltax = x1 - x0
    deltay = y1 - y0
    error = -1.0
    deltaerr = abs(deltay / deltax)
        # note that this division needs to be done in a way that preserves the fractional part
    y = y0
    for x in range(int(x0), int(x1)-1):
        display.pixel(x, y, 1)
        error = error + deltaerr
        if error >= 0.0:
            y = y + 1
            error = error - 1.0

def draw_circle(display, x0, y0, radius):
    x = radius
    y = 0
    err = 0

    while x >= y:
        display.pixel(x0 + x, y0 + y, 1)
        display.pixel(x0 + y, y0 + x, 1)
        display.pixel(x0 - y, y0 + x, 1)
        display.pixel(x0 - x, y0 + y, 1)
        display.pixel(x0 - x, y0 - y, 1)
        display.pixel(x0 - y, y0 - x, 1)
        display.pixel(x0 + y, y0 - x, 1)
        display.pixel(x0 + x, y0 - y, 1)

        y += 1
        err += 1 + 2*y
        if 2*(err-x) + 1 > 0:
            x -= 1
            err += 1 - 2*x
            
i2c = I2C(sda=Pin(4), scl=Pin(5))
print("I2C scan: ", i2c.scan())
display = ssd1306.SSD1306_I2C(64, 48, i2c)

def boot_logo(display):
  # draw boot logo
  display.fill(0)    
  display.show()
  for y, row in enumerate(logo.pic):
      for x, col in enumerate(row):
          if col == "1":
              display.pixel(x, y, 1)
            
  display.show()
  utime.sleep(5)
  display.fill(0)    
  display.show()

boot_logo(display)

# connect to WiFi
do_connect()

def mqtt_connect():
    client = MQTTClient(CONFIG['client_id'], CONFIG['broker'])
    client.connect()
    print("Connected to {}".format(CONFIG['broker']))    

while client == None:
  print("Waiting for MQTT connect...")
  mqtt_connect()
  sleep(5)

# main loop
while True: 
  display.fill(0)
  display.show()

  display.text('WiFi IP',0,10)
  display.show()

  ifconfig = wlan.ifconfig()
  display.text(ifconfig[0],-40,20)
  display.show()
  utime.sleep(5)

  display.fill(0)
  display.show()
  adc = ADC(0)
  

  counter = 60

  def callback(p):
    print('pin change', p)

  d0 = Pin(0, Pin.IN)     # create input pin on GPIO2
  d2 = Pin(2, Pin.IN)     # create input pin on GPIO2
  d3 = Pin(3, Pin.IN)     # create input pin on GPIO2
  
  d0.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=callback)
  d2.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=callback)
  d3.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=callback)
    
  while True:

    counter -= 1

    adc_value = adc.read()
  
    var = "ADC {}".format(adc_value) # 1-4 chars
    display.text(var,0,10)
    display.show()
    data = adc_value
    client.publish('{}/{}'.format(CONFIG['topic'], 'ADC'), bytes(str(data), 'utf-8'))
      
    print('D0:',d0.value())
    display.text('D0     {}'.format(d0.value()),0,20)
    display.show()
    data = d0.value()
    client.publish('{}/{}'.format(CONFIG['topic'], 'D0'), bytes(str(data), 'utf-8'))
    print('Sensor state: {}'.format(data))

    print('D2:',d2.value())
    display.text('D2     {}'.format(d2.value()),0,30)
    data = d2.value()
    client.publish('{}/{}'.format(CONFIG['topic'], 'D2'), bytes(str(data), 'utf-8'))
    display.show()
    
    print('D3:',d3.value())
    data = d3.value()
    display.text('D3     {}'.format(d3.value()),0,40)
    client.publish('{}/{}'.format(CONFIG['topic'], 'D3'), bytes(str(data), 'utf-8'))
    display.show()
    
    utime.sleep(1)

    print(counter)

    if counter == 0:
      counter = 10
      boot_logo(display)
