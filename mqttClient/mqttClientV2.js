var http = require('http');
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
 	clientId: 'server',
 	});

// Membangun koneksi dengan broker
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
	database: "smart_algae_pond"
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
	message_str = message_str.replace(/\n$/, ''); // menghilangkan new line
	//payload syntaxnya ada clientID, topic, dan message
	if (countInstances(message_str) != 1) { //data yang dikirim sesuai atau tidak
		console.log("Invalid Payload");
		} else {
		insert_message(topic, message_str, packet);
		console.log(message_str);
	}
};

// fungsi insert data ke setiap kolomnya
function insert_message(topic, message_str, packet) {
    var message_arr = extract_string(message_str); // ngesplit string
    var kolamId = Number(message_arr[0]);
    // fungsi untuk GET id_produksi dari API
    const options = {
      hostname: '117.53.47.76',
      path: '/backend-mikroalga/public/api/kolam/' + kolamId +'/latest_produksi',
      method: 'GET'
    }

    const req = http.request(options, res => {
    var id_produksi = '';
          res.on('data', d => {
            status = res.statusCode;
            var data = JSON.parse(d);
            if (status == "200") {
                for (var key in data) {
                        try {
                        data[key] = JSON.parse(data[key]);
                        } catch(err) {
                        }
                }
                var hasil = data['data']['id_produksi'];
                id_produksi += hasil;
           }
            else {
                process.stdout.write('id_kolam tidak ditemukan' + '\n')
            }

        var ph_air = Number(message_arr[1]); //
        var aliran_udara = Number(message_arr[2]); //
        var kecepatan_air = Number(message_arr[3]); //
        var intensitas_cahaya = Number(message_arr[4]); //
        var energi_listrik = Number(message_arr[5]); //
        var suhu_air = Number(message_arr[6]); //
        var kekeruhan = Number(message_arr[7]); //
        var ketinggian_air = Number(message_arr[8]); //
        var created_at = new Date(message_arr[9]); //


        var sql = "INSERT INTO ?? (??,??,??,??,??,??,??,??,??,??) VALUES (?,'?','?','?','?','?','?','?','?',?)";
        var params = ['output_sensor', 'id_produksi', 'suhu_air', 'ketinggian_air', 'kecepatan_air', 'ph_air', 'kekeruhan', 'aliran_udara', 'intensitas_cahaya', 'energi_listrik', 'created_at', id_produksi, suhu_air, ketinggian_air, kecepatan_air, ph_air, kekeruhan, aliran_udara, intensitas_cahaya, energi_listrik, created_at];
        sql = mysql.format(sql, params);

        connection.query(sql, function (error, results) {
                if (error) throw error;
                console.log("Success");
            })
          })
        })

    req.end();
}


function extract_string(message_str) {
	var message_arr = message_str.split(","); //convert to array
	return message_arr;
};

// fungsi menghapus delimeter
var delimiter = " ";
function countInstances(message_str) {
	var substrings = message_str.split(delimiter);
	return substrings.length - 1;
};


