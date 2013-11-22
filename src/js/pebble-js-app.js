// Define the initial config default values.  These defaults are used
// only if the Pebble is connected to the phone at the time of launch;
// otherwise, the defaults in init_default_options() are used instead.
var default_keep_battery_gauge = 0;
var default_keep_bluetooth_indicator = 0;
var default_second_hand = 0;
var default_hour_buzzer = 0;
var default_hurt = 1;

var keep_battery_gauge = localStorage.getItem("doctors:keep_battery_gauge");
if (!keep_battery_gauge) {
    keep_battery_gauge = default_keep_battery_gauge;
}
keep_battery_gauge = parseInt(keep_battery_gauge);

var keep_bluetooth_indicator = localStorage.getItem("doctors:keep_bluetooth_indicator");
if (!keep_bluetooth_indicator) {
    keep_bluetooth_indicator = default_keep_battery_gauge;
}
keep_bluetooth_indicator = parseInt(keep_bluetooth_indicator);

var second_hand = localStorage.getItem("doctors:second_hand");
if (!second_hand) {
    second_hand = default_second_hand;
}
second_hand = parseInt(second_hand);

var hour_buzzer = localStorage.getItem("doctors:hour_buzzer");
if (!hour_buzzer) {
    hour_buzzer = default_hour_buzzer;
}
hour_buzzer = parseInt(hour_buzzer);

var hurt = localStorage.getItem("doctors:hurt");
if (!hurt) {
    hurt = default_hurt;
}
hurt = parseInt(hurt);

Pebble.addEventListener("ready", function() {
    console.log("ready");
    console.log("   keep_battery_gauge: " + keep_battery_gauge);
    console.log("   keep_bluetooth_indicator: " + keep_bluetooth_indicator);
    console.log("   second_hand: " + second_hand);
    console.log("   hour_buzzer: " + hour_buzzer);
    console.log("   hurt: " + hurt);

	// At startup, send the current configuration to the Pebble--the
	// phone storage keeps the authoritative state.  We delay by 1
	// second to give the Pebble a chance to set itself up for
	// receiving messages.
	setTimeout(function() {
		var configuration = {
			'keep_battery_gauge' : keep_battery_gauge,
			'keep_bluetooth_indicator' : keep_bluetooth_indicator,
			'second_hand' : second_hand,
			'hour_buzzer' : hour_buzzer,
			'hurt' : hurt,
		};
  	    console.log("sending init config: " + JSON.stringify(configuration));
		Pebble.sendAppMessage(configuration);
	}, 1000)

    initialized = true;
});

Pebble.addEventListener("showConfiguration", function(e) {
    var url = "http://www.ddrose.com/pebble/doctors_configure.html?keep_battery_gauge=" + keep_battery_gauge + "&keep_bluetooth_indicator=" + keep_bluetooth_indicator + "&second_hand=" + second_hand + "&hour_buzzer=" + hour_buzzer + "&hurt=" + hurt;
    console.log("showConfiguration: " + url);
    var result = Pebble.openURL(url);
    console.log("openURL result: " + result);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("Configuration window closed");
    console.log(e.type);
    console.log(e.response);

    var configuration = JSON.parse(e.response);
  	console.log("sending runtime config: " + JSON.stringify(configuration));
    Pebble.sendAppMessage(configuration);
    
    keep_battery_gauge = configuration["keep_battery_gauge"];
    localStorage.setItem("doctors:keep_battery_gauge", keep_battery_gauge);
    
    keep_bluetooth_indicator = configuration["keep_bluetooth_indicator"];
    localStorage.setItem("doctors:keep_bluetooth_indicator", keep_bluetooth_indicator);
    
    second_hand = configuration["second_hand"];
    localStorage.setItem("doctors:second_hand", second_hand);
    
    hour_buzzer = configuration["hour_buzzer"];
    localStorage.setItem("doctors:hour_buzzer", hour_buzzer);
    
    hurt = configuration["hurt"];
    localStorage.setItem("doctors:hurt", hurt);
});
