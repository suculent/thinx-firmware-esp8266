// Dependencies:
// npm install mqtt --save
// npm install mdns --save
// npm install slack-node --save

// CONFIGURATION

var broker_address = 'mqtt://localhost'
var mdns_service_name = 'mqtt2slack'
var mqtt_channel = "/fridge"

var querystring = require('querystring');
var http = require('http');

// MDNS

var mdns = require('mdns')
var mqtt = require('mqtt')
var client

var ad = mdns.createAdvertisement(mdns.tcp(mdns_service_name), 1883)
ad.start()
console.log("MDNS advertising: " + mdns_service_name)

// watch all http servers 
var browser = mdns.createBrowser(mdns.tcp(mdns_service_name))
browser.on('serviceUp', function(service) {
  console.log(mdns_service_name + " service up: ", service)  
});

browser.on('serviceDown', function(service) {
  console.log(mdns_service_name + " service down: ", service)
});

browser.stop();

// MQTT CONNECT / RESPOND


function process_message(topic, message) {
	
	console.log("message: " + message.toString())

	if (topic == (mqtt_channel + "/open")) {
		var j = JSON.parse(message)
		if (j) {			
			var interval = parseInt(j.interval)
			if (interval) {
				console.log("[processing] " + mqtt_channel + "is open for " + interval + " seconds.")

				if (interval > 60) {
					if (interval >= 61 && interval < 62) {
						send_slack("Jsem otevřená již minutu.")
					} else if (interval == 120) {
						send_slack("Jsem otevřená již dvě minuty.")
					} else if (interval >= 240) {
						send_slack("Jsem otevřená již " + interval + " vteřin.")
					}
				} else {
					//send_slack("Jsem otevrena.")
				}

			}

		}

		console.log("[processing] " + mqtt_channel + " is open.")
	}

	if (topic == (mqtt_channel + "/closed")) {
		console.log("[processing] " + mqtt_channel + " is closed.")
	}

}

function init_mqtt(broker_address, channel) {		
	console.log("MQTT connecting to: " + broker_address)
	client = mqtt.connect(broker_address)

	client.on('connect', function () {
		console.log("MQTT connected, subscribing to: " + channel)
		client.subscribe(channel + "/open")
		client.subscribe(channel + "/closed")
		client.subscribe(channel) // loopback only
		client.publish(channel, 'MQTT->Slack Gateway Started...')
	})
	 
	client.on('message', function (topic, message) {
	  // message is Buffer 
	  process_message(topic, message)	 
	  //client.end()
	})
}
 
function send_slack(message) {
  
  slack.webhook({
	  channel: "#home",
	  username: "lednicka",
	  text: message
	}, function(err, response) {
	  console.log(response);
	});
}

var Slack = require('slack-node'); 
webhookUri = "https://hooks.slack.com/services/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"; 
slack = new Slack();
slack.setWebhook(webhookUri);

console.log("Initializing " + broker_address + " MQTT broker with base channel " + mqtt_channel)
init_mqtt(broker_address, mqtt_channel)
