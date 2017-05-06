function connect(ssid, password)
    wifi.setmode(wifi.STATION)
    wifi.sta.config(ssid, password)
    wifi.sta.connect()
    tmr.alarm(1, 1000, 1, function()
        if wifi.sta.getip() == nil then
            print("Connecting " .. ssid .. "...")
        else
            tmr.stop(1)
            print("Connected to " .. ssid .. ", IP is "..wifi.sta.getip())
            m("192.168.1.9")
        end
    end)
end

function m(target)
    local m = mqtt.Client(node.chipid(), 120, "username", "password")
    m:lwt("/lwt", "offline", 0, 0)
    m:on("connect", function(client) 
        print ("connected") 
    end)
    m:on("offline", function(client) 
        print ("offline") 
        m:close();        
        connect(wifi_ssid, wifi_password)
    end)

    m:on("message", function(client, topic, data) 
        print(topic .. ":" ) 
        if data ~= nil then print("message: " .. data) end
    end)

    print("Connecting to MQTT to " .. target .. "...")
    m:connect(target, 1883, 0,
    function(client)
        print(":mconnected") 
        m:subscribe("/fridge",0, function(client) print("subscribe success") end)
        local initial_state = tonumber(gpio.read(pin))
        local current_state = initial_state
        print("Initial state (must be closed on reset): " .. initial_state)
        m:publish("/fridge",initial_state,0,0)
        local counter = 0
        local last_state = intial_state
        tmr.alarm(1, 30000, 1, function()
            local current = tonumber(gpio.read(pin))            
            if current == 1 then
                if last_state == 0 then
                    local message = '{ "interval" : ' .. tostring(counter/1000) .. '}'
                    m:publish("/fridge/closed",message,0,0)                 
                end
                counter = 0
                --print(".")
                last_state = 1
            else
                counter = counter + 30000
                local message = '{ "interval" : ' .. tostring(counter/1000) .. '}'
                m:publish("/fridge/open",message,0,0)                
                print("Fridge door open: " .. tostring(current) .. " for " .. counter/1000 .. "seconds")                
                current_state = 0
                last_state = 0
            end
            current_state = current
            collectgarbage()
        end)
    end,     
    function(client, reason)
        print("failed reason: "..reason)
    end
)

end

dofile("config.lua")
wifi_ssid = "page42.showflake.czf"
wifi_password = "quarantine"
pin = 0
gpio.mode(pin,gpio.INPUT)
led = 2
gpio.mode(led,gpio.OUTPUT)
gpio.write(led,gpio.HIGH)
connect(wifi_ssid, wifi_password)
