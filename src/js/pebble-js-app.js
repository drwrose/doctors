// Define the initial config default values.  These defaults are used
// only if the Pebble is connected to the phone at the time of launch;
// otherwise, the defaults in init_default_options() are used instead.
var default_keep_battery_gauge = 0;
var default_keep_bluetooth_indicator = 0;
var default_second_hand = 0;
var default_hour_buzzer = 0;
var default_bluetooth_buzzer = 1;
var default_hurt = 1;
var default_show_date = 0;
var default_display_lang = 'en_US';

var keep_battery_gauge;
var keep_bluetooth_indicator;
var second_hand;
var hour_buzzer;
var bluetooth_buzzer;
var hurt;
var show_date;
var display_lang;

function sent_ack(e) {
    console.log("Message sent");
}

function sent_nack(e) {
    console.log("Message not sent: " + e.error);
    //console.log(e.error.message);
}

function logLocalStorage() {
    var keys = Object.keys(localStorage);
    console.log("  localStorage = {");
    for (var key in keys) {
	console.log('      "' + keys[key] + '" : ' + localStorage[keys[key]] + ',');
    }
    console.log("  }");
}

function getIntFromStorage(keyword, default_value) {
    var value = localStorage.getItem("doctors:" + keyword);
    if (!value) {
	value = default_value;
    }
    value = parseInt(value);
    if (isNaN(value)) {
	value = default_value;
    }

    console.log("   " + keyword + ": " + value);
    return value;
}

function getStringFromStorage(keyword, default_value) {
    var value = localStorage.getItem("doctors:" + keyword);
    if (!value) {
	value = default_value;
    }

    console.log("   " + keyword + ": '" + value + "'");
    return value;
}

var initialized = false;
function initialize() {
    console.log("initialize: " + initialized);
    if (initialized) {
	return;
    }

    logLocalStorage();
    keep_battery_gauge = getIntFromStorage('keep_battery_gauge', default_keep_battery_gauge);
    keep_bluetooth_indicator = getIntFromStorage('keep_bluetooth_indicator', default_keep_bluetooth_indicator);
    second_hand = getIntFromStorage('second_hand', default_second_hand);
    hour_buzzer = getIntFromStorage('hour_buzzer', default_hour_buzzer);
    bluetooth_buzzer = getIntFromStorage('bluetooth_buzzer', default_bluetooth_buzzer);
    hurt = getIntFromStorage('hurt', default_hurt);
    show_date = getIntFromStorage('show_date', default_show_date);
    display_lang = getStringFromStorage('display_lang', default_display_lang);

    console.log("   keep_battery_gauge: " + keep_battery_gauge);
    console.log("   keep_bluetooth_indicator: " + keep_bluetooth_indicator);
    console.log("   second_hand: " + second_hand);
    console.log("   hour_buzzer: " + hour_buzzer);
    console.log("   bluetooth_buzzer: " + bluetooth_buzzer);
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
	    'bluetooth_buzzer' : bluetooth_buzzer,
	    'hurt' : hurt,
	    'show_date' : show_date,
	    'display_lang' : display_lang,
	};
  	console.log("sending init config: " + JSON.stringify(configuration));
	Pebble.sendAppMessage(configuration, sent_ack, sent_nack);
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
    var url = "http://www.ddrose.com/pebble/doctors_2_6_2_configure.html?keep_battery_gauge=" + keep_battery_gauge + "&keep_bluetooth_indicator=" + keep_bluetooth_indicator + "&second_hand=" + second_hand + "&hour_buzzer=" + hour_buzzer + "&bluetooth_buzzer=" + bluetooth_buzzer + "&hurt=" + hurt + "&show_date=" + show_date + "&display_lang=" + display_lang;
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
	Pebble.sendAppMessage(configuration, sent_ack, sent_nack);
	
	keep_battery_gauge = configuration["keep_battery_gauge"];
	localStorage.setItem("doctors:keep_battery_gauge", keep_battery_gauge);
	
	keep_bluetooth_indicator = configuration["keep_bluetooth_indicator"];
	localStorage.setItem("doctors:keep_bluetooth_indicator", keep_bluetooth_indicator);
	
	second_hand = configuration["second_hand"];
	localStorage.setItem("doctors:second_hand", second_hand);
	
	hour_buzzer = configuration["hour_buzzer"];
	localStorage.setItem("doctors:hour_buzzer", hour_buzzer);
	
	bluetooth_buzzer = configuration["bluetooth_buzzer"];
	localStorage.setItem("doctors:bluetooth_buzzer", bluetooth_buzzer);
	
	hurt = configuration["hurt"];
	localStorage.setItem("doctors:hurt", hurt);
	
	show_date = configuration["show_date"];
	localStorage.setItem("doctors:show_date", show_date);
	
	display_lang = configuration["display_lang"];
	localStorage.setItem("doctors:display_lang", display_lang);

	logLocalStorage();
    }
});
