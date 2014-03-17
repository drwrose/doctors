// Define the initial config default values.  These defaults are used
// only if the Pebble is connected to the phone at the time of launch;
// otherwise, the defaults in init_default_options() are used instead.
var default_keep_battery_gauge = 0;
var default_keep_bluetooth_indicator = 0;
var default_second_hand = 0;
var default_hour_buzzer = 0;
var default_hurt = 1;
var default_show_date = 0;
var default_display_lang = 0;

var keep_battery_gauge;
var keep_bluetooth_indicator;
var second_hand;
var hour_buzzer;
var hurt;
var show_date;
var display_lang;

var logLocalStorage = function() {
    var keys = Object.keys(localStorage);
    console.log("  localStorage = {");
    for (var key in keys) {
	console.log('      "' + keys[key] + '" : ' + localStorage[keys[key]] + ',');
    }
    console.log("  }");
}

var initialized = false;
var initialize = function() {
    console.log("initialize: " + initialized);
    if (initialized) {
	return;
    }

    logLocalStorage();

    keep_battery_gauge = localStorage.getItem("doctors:keep_battery_gauge");
    if (!keep_battery_gauge) {
	keep_battery_gauge = default_keep_battery_gauge;
    }
    keep_battery_gauge = parseInt(keep_battery_gauge);

    keep_bluetooth_indicator = localStorage.getItem("doctors:keep_bluetooth_indicator");
    if (!keep_bluetooth_indicator) {
	keep_bluetooth_indicator = default_keep_battery_gauge;
    }
    keep_bluetooth_indicator = parseInt(keep_bluetooth_indicator);

    second_hand = localStorage.getItem("doctors:second_hand");
    if (!second_hand) {
	second_hand = default_second_hand;
    }
    second_hand = parseInt(second_hand);

    hour_buzzer = localStorage.getItem("doctors:hour_buzzer");
    if (!hour_buzzer) {
	hour_buzzer = default_hour_buzzer;
    }
    hour_buzzer = parseInt(hour_buzzer);

    hurt = localStorage.getItem("doctors:hurt");
    if (!hurt) {
	hurt = default_hurt;
    }
    hurt = parseInt(hurt);

    show_date = localStorage.getItem("doctors:show_date");
    if (!show_date) {
	show_date = default_show_date;
    }
    show_date = parseInt(show_date);

    display_lang = localStorage.getItem("doctors:display_lang");
    if (!display_lang) {
	display_lang = default_display_lang;
    }
    display_lang = parseInt(display_lang);

    console.log("   keep_battery_gauge: " + keep_battery_gauge);
    console.log("   keep_bluetooth_indicator: " + keep_bluetooth_indicator);
    console.log("   second_hand: " + second_hand);
    console.log("   hour_buzzer: " + hour_buzzer);
    console.log("   hurt: " + hurt);
    console.log("   show_date: " + show_date);
    console.log("   display_lang: " + display_lang);

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
	    'show_date' : show_date,
	    'display_lang' : display_lang,
	};
  	console.log("sending init config: " + JSON.stringify(configuration));
	Pebble.sendAppMessage(configuration);
    }, 1000)
    
    initialized = true;
};

Pebble.addEventListener("ready", function(e) {
    console.log("ready");
    initialize();
});

Pebble.addEventListener("showConfiguration", function(e) {
    console.log("showConfiguration starting");
    initialize();
    var url = "http://www.ddrose.com/pebble/doctors_2_4_configure.html?keep_battery_gauge=" + keep_battery_gauge + "&keep_bluetooth_indicator=" + keep_bluetooth_indicator + "&second_hand=" + second_hand + "&hour_buzzer=" + hour_buzzer + "&hurt=" + hurt + "&show_date=" + show_date + "&display_lang=" + display_lang;
    console.log("showConfiguration: " + url);
    var result = Pebble.openURL(url);
    console.log("openURL result: " + result);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("Configuration window closed");
    console.log(e.type);
    console.log(e.response);

    if (e.response && e.response != 'CANCELLED') {
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
	
	show_date = configuration["show_date"];
	localStorage.setItem("doctors:show_date", show_date);
	
	display_lang = configuration["display_lang"];
	localStorage.setItem("doctors:display_lang", display_lang);

	logLocalStorage();
    }
});
