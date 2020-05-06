var awsIot = require('aws-iot-device-sdk');
var mysql = require('mysql');
var Database_URL = 'localhost';
// var Database_URL = '117.53.47.76';
var Topic = 'algaepond/1/monitoring';
var device = awsIot.device({
 	keyPath: 'cert/private.pem.key',
 	certPath: 'cert/cer.pem.crt',
 	caPath: 'cert/root.pem',
 	host: 'a22eoammq4iudf-ats.iot.us-east-1.amazonaws.com',
 	clientId: 'database_AlgaePond',
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
	password: "d4tabas34dm1n2S31s1pb",
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
	message_str = message_str.replace(/\n$/, ''); //remove new line
	//payload syntax: clientID,topic,message
	if (countInstances(message_str) != 1) {		//data yang dikirim sesuai atau tidak
		console.log("Invalid Payload");
		} else {
		insert_message(topic, message_str, packet);
		//console.log(message_arr);
	}
};

//insert a row into the tbl_messages table
function insert_message(topic, message_str, packet) {
	var message_arr = extract_string(message_str); // ngesplit string
    var ph = Number(message_arr[0]); //
    var mpx = Number(message_arr[1]); //
    var water_flow = Number(message_arr[2]); //
    var lux = Number(message_arr[3]); // dalam lux
    var pzem = Number(message_arr[4]); // dalam VA
    var suhu_air = Number(message_arr[5]); // dalam *c
    var turbidity = Number(message_arr[6]); //
    var water_lv = Number(message_arr[7]); //
    var tanggal = new Date(message_arr[8]); //

	var sql = "INSERT INTO ?? (??,??,??,??,??,??,??,??,??) VALUES (?,?,?,?,?,?,?,?,?)";
	var params = ['output_sensor','ph','mpx','water_flow', 'lux', 'pzem','suhu_air', 'turbidity', 'water_lv', 'tanggal', ph, mpx, water_flow, lux, pzem, suhu_air, turbidity, water_lv, tanggal];
	sql = mysql.format(sql, params);

	connection.query(sql, function (error, results) {
		if (error) throw error;
		console.log("Success");
	});
};

// fungsi untuk ngesplit data
function extract_string(message_str) {
	var message_arr = message_str.split(","); //convert to array
	return message_arr;
};

// menghitung jumlah delimeter
var delimiter = " ";
function countInstances(message_str) {
	var substrings = message_str.split(delimiter);
	return substrings.length - 1;
};
