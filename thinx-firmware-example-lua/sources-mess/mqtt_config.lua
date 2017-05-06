-- file : mqtt_config.lua
local module = {}

module.SSID = {}  
module.SSID["page42.showflake.czf"] = "quarantine"

module.HOST = "192.168.1.9"  
module.PORT = 1884  
module.ID = node.chipid()

module.ENDPOINT = "nodemcu/"  
return module  