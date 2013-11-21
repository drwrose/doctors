var hour_buzzer = localStorage.getItem("hour_buzzer");
if (!hour_buzzer) {
	hour_buzzer = 0;
}

var hurt = localStorage.getItem("hurt");
if (!hurt) {
	hurt = 0;
}

var colon = localStorage.getItem("colon");
if (!colon) {
	colon = 0;
}

Pebble.addEventListener("ready", function() {
	console.log("ready");
	console.log("	hour_buzzer: " + hour_buzzer);
	console.log("	hurt: " + hurt);
	console.log("	colon: " + colon);

	// At startup, send the current configuration to the Pebble--the
	// phone storage keeps the authoritative state.
	var configuration = {
		'hour_buzzer' : str(hour_buzzer),
		'hurt' : str(hurt),
		'colon' : str(colon),
	};
	Pebble.sendAppMessage(configuration);

	initialized = true;
});

Pebble.addEventListener("showConfiguration", function(e) {
	var url = "http://www.ddrose.com/pebble/doctors_configure.html?hour_buzzer=" + hour_buzzer + "&hurt=" + hurt + "&colon=" + colon;
	console.log("showConfiguration: " + url);
	var result = Pebble.openURL(url);
	console.log("openURL result: " + result);
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("Configuration window closed");
	console.log(e.type);
	console.log(e.response);

	var configuration = JSON.parse(e.response);
	Pebble.sendAppMessage(configuration);
	
	hour_buzzer = configuration["hour_buzzer"];
	localStorage.setItem("hour_buzzer", hour_buzzer);
	
	hurt = configuration["hurt"];
	localStorage.setItem("hurt", hurt);
	
	colon = configuration["colon"];
	localStorage.setItem("colon", colon);
});
