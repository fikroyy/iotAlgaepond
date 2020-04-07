var awsIot = require('aws-iot-device-sdk');
var mysql = require('mysql');
// var sleep = require('sleep');
var Database_URL = 'localhost';
// var Database_URL = '117.53.47.76';
var Topic = 'algaepond/monitoring';
var device = awsIot.device({
 	keyPath: 'cert/private.pem.key',
 	certPath: 'cert/cer.pem.crt',
 	caPath: 'cert/root.pem',
 	host: 'a22eoammq4iudf-ats.iot.us-east-1.amazonaws.com',
 	clientId: 'server',
 	});

 device.on('connect', function() {
 	console.log('Connected');
 	device.subscribe(Topic, mqtt_subscribe);
 });
 device.on('message', mqtt_messsageReceived);

// Koneksi ke database
var connection = mysql.createConnection({
	host: Database_URL,
	port : 3306,
	user: "root",
	password: "4dm1n2S31s1pb",
	database: "algaePond"
});

connection.connect(function(err) {
	if (err) throw err
;	console.log("Database Connected!");
});

function mqtt_subscribe(err, granted) {
    console.log("Subscribed to " + Topic);
    if (err) {console.log(err);}
};
 function mqtt_messsageReceived(topic, message, packet) {
	var message_str = message.toString(); //convert byte array to string
	message_str = message_str.replace(/\n$/, ''); // hilangin new line
	//payload syntax: clientID,topic,message
	if (countInstances(message_str) != 1) {		//data yang dikirim sesuai atau tidak
		console.log("Invalid Payload");
		} else {
		insert_message(topic, message_str, packet);
		//console.log(message_arr);
	}
};

// masukin data ke database, query masing masing sesuai alat sensornya
function insert_message(topic, message_str, packet) {
	var message_arr = extract_string(message_str); // ngesplit string
	var ph = Number(message_arr[0]); //
	var mpx = Number(message_arr[1]); //
	var water_flow = Number(message_arr[2]); //
	var lux = Number(message_arr[3]); //
	var pzem = Number(message_arr[4]); //
	var suhu_air = Number(message_arr[5]); //
	var turbidity = Number(message_arr[6]); //
	var water_lv = Number(message_arr[7]); //
	var tanggal = new Date(message_arr[8]); //

	var query = "INSERT INTO ?? (??,??) VALUES (?,?)";
	var params = ['ph','ph','tanggal', ph, tanggal];
	query = mysql.format(query, params);

	var query1 = "INSERT INTO ?? (??,??) VALUES (?,?)";
	var params = ['mpx','mpx','tanggal', mpx, tanggal];
	query1 = mysql.format(query1, params);

	var query2 = "INSERT INTO ?? (??,??) VALUES (?,?)";
	var params = ['water_flow','water_flow','tanggal', water_flow, tanggal];
	query2 = mysql.format(query2, params);

	var query3 = "INSERT INTO ?? (??,??) VALUES (?,?)";
	var params = ['lux','lux','tanggal', lux, tanggal];
	query3 = mysql.format(query3, params);

	var query4 = "INSERT INTO ?? (??,??) VALUES (?,?)";
       	var params = ['pzem','pzem','tanggal', pzem, tanggal];
 	query4 = mysql.format(query4, params);

	var query5 = "INSERT INTO ?? (??,??) VALUES (?,?)";
        var params = ['suhu_air','suhu_air','tanggal', suhu_air, tanggal];
	query5 = mysql.format(query5, params);

	var query6 = "INSERT INTO ?? (??,??) VALUES (?,?)";
        var params = ['turbidity','turbidity','tanggal', turbidity, tanggal];
	query6 = mysql.format(query6, params);

	var query7 = "INSERT INTO ?? (??,??) VALUES (?,?)";
	var params = ['water_lv','water_lv','tanggal', water_lv, tanggal];
	query7 = mysql.format(query7, params);

	connection.query(query, function (error, results) {
		if (error) throw error;
		console.log("Success");
	});
	connection.query(query1, function (error, results) {
		if (error) throw error;
		console.log("Success");
	});
	connection.query(query2, function (error, results) {
		if (error) throw error;
		console.log("Success");
	});
	connection.query(query3, function (error, results) {
		if (error) throw error;
		console.log("Success");
	});
	connection.query(query4, function (error, results) {
                if (error) throw error;
                console.log("Success");
        });
	connection.query(query5, function (error, results) {
                if (error) throw error;
                console.log("Success");
        });
	connection.query(query6, function (error, results) {
                if (error) throw error;
                console.log("Success");
        });
	connection.query(query7, function (error, results) {
                if (error) throw error;
                console.log("Success");
        });
};

//split a string into an array of substrings
function extract_string(message_str) {
	var message_arr = message_str.split(","); //convert to array
	return message_arr;
};

//count number of delimiters in a string
var delimiter = " ";
function countInstances(message_str) {
	var substrings = message_str.split(delimiter);
	return substrings.length - 1;
};
