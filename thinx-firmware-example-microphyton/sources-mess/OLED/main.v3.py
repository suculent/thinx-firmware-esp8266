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

CONFIG = {
    "broker": "192.168.1.21",
    "sensor_pin": 0, 
    "client_id": b"esp8266_" + ubinascii.hexlify(machine.unique_id()),
    "topic": "/display/2",
}

client = None

def do_connect():        
    wlan.active(True)
    if not wlan.isconnected():
        print('connecting to network...')
        wlan.connect('page42.showflake.czf', 'quarantine')
        while not wlan.isconnected():
            pass
    print('network config:', wlan.ifconfig())
    mqtt_connect()

def mqtt_connect():
  global client
  client = MQTTClient(CONFIG['client_id'], CONFIG['broker'])
  client.connect()
  print("Connected to MQTT broker {}".format(CONFIG['broker']))    
  while client == None:
    pass

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

print('machine frequency: ', machine.freq()/1000000, ' MHz')
wlan = network.WLAN(network.STA_IF)

def do_ap():
  ap = network.WLAN(network.AP_IF)
  ap.active(True)
  ap.config(essid='ESP-AP')
            
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
do_connect()

def callback(p):
  print('pin change', p)

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

  d2 = Pin(12, Pin.IN)
  d3 = Pin(13, Pin.IN)
  # pin 4 and 5 is used for OLED I2C BUS (D1/D2)
  d6 = Pin(14, Pin.IN)    
  d2.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=callback)
  d3.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=callback)
  d6.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=callback)
    
  while True:
    counter -= 1
    display.fill(0)
    display.show()  
  
    adc_data = adc.read()
    display.text("ADC {}".format(adc_data, '04d'),0,10)
    display.show()        
    message = '\{ "MQ135ADC" : {} \}'.format(str(adc_data))
    print(message)
    topic = '{}/{}'.format(CONFIG['topic'], 'ADC')
    client.publish(topic, bytes(message, 'utf-8'))    
    
    data = d2.value()
    display.text('D12    {}'.format(data),0,30)    
    display.show()
    message = '\{ "MQ135" : \{ "ADC" : {}, "D" : {} \}'.format(str(adc_data), str(data))
    print(message)
    topic = '{}/{}'.format(CONFIG['topic'], 'D12')
    client.publish(topic, bytes(str(data), 'utf-8'))
          
    data = d3.value()
    display.text('D13    {}'.format(data),0,40)
    display.show()
    message = '\{ "MQ3" : {} \}'.format(str(data))
    topic = '{}/{}'.format(CONFIG['topic'], 'D13')
    client.publish(topic, bytes(str(data), 'utf-8'))    
    
    data = d6.value()
    display.text('D14    {}'.format(d6.value()),0,20)
    display.show()
    message = '\{ "MQ7" : {} \}'.format(str(data))
    print(message)
    topic = '{}/{}'.format(CONFIG['topic'], 'D14')
    client.publish(topic, bytes(str(data), 'utf-8'))
    
    utime.sleep(5)
    print(counter)
    if counter == 0:
      counter = 10
      boot_logo(display)
