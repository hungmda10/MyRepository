var mysql = require('mysql');
var mqtt  = require('mqtt');
var express = require('express');
var session = require('express-session');
var bodyParser = require('body-parser');
var path = require('path');
var app = express();

var count = 0;
var client = mqtt.connect("mqtt://localhost:1883",{clientId:"mqttjs01",username:"root",password:"root"});
var topic1 = "Topic 1";
var topic2 = "Topic 2";
var message="test message";
var topic_list=["home/sensors/temperature","home/sensors/humidity","home/sensors/Gas"];

console.log("connected flag  " + client.connected);


function publish(topic,msg,options){
	console.log("publishing",msg);
	
	if (client.connected == true){
		client.publish(topic,msg,options);
	}
}

function getDateTime() {

    var date = new Date();

    var hour = date.getHours();
    hour = (hour < 10 ? "0" : "") + hour; 

    var min  = date.getMinutes();
    min = (min < 10 ? "0" : "") + min;

    var sec  = date.getSeconds();
    sec = (sec < 10 ? "0" : "") + sec;

    var year = date.getFullYear();

    var month = date.getMonth() + 1;
    month = (month < 10 ? "0" : "") + month;

    var day  = date.getDate();
    day = (day < 10 ? "0" : "") + day;

    return year + ":" + month + ":" + day + ":" + hour + ":" + min + ":" + sec;

}

var server = app.listen(3000, () => { //Start the server, listening on port 3000.
    console.log("Conect to requests on port 3000...");
})

var connection = mysql.createConnection({
	host     : 'localhost',
	user     : 'root',
	password : 'root',
	database : 'MQTT'
});

connection.connect(function(err) {
	if (err) 
		throw err;
	console.log("mysql connected");
	var sql ="DROP TABLE IF EXISTS sensors";
	connection.query(sql, function(err, result){
		if (err) 
			throw err;
		console.log("drop tables sensors ok");
	});
	sql = "CREATE TABLE sensors( id INT(10) PRIMARY KEY  auto_increment,Sensor_ID varchar(10) not null,Date_and_Time datetime not null,Temperature int(3) not null,Humidity int(3) not null,Gas int(3) not null)"
	connection.query(sql, function(err, result){
		if (err) 
			throw err;
		console.log("create tables sensors ok");
	});
});




app.get('/', function(request, response) {
	response.sendFile(path.join(__dirname + '/final.html'));
});

var io = require('socket.io')(server); //Bind socket.io to our express server.

//app.use(express.static('public')); //Send index.html page on GET /

io.on('connection', (socket) => {
    console.log("Someone connected."); //show a log as a new client connects.
    var today = new Date();
	 connection.query("SELECT * FROM sensors", function (err, result, fields) {
	 if (err) throw err;
	 result.forEach(function(value) {
	 var m_time = getDateTime();
	console.log(m_time);
    io.sockets.emit('temp', { time:m_time , temp:value.Temperature, hum:value.Humidity, gas:value.Gas}); 
	 });
	 
	});
	
})
var Temp ;
var Hum ;
var Gas ;
var cnt_check = 0;

client.on('message',function(topic, message, packet){
	console.log("topic is "+ topic);
	//message = JSON.parse(message);
	if( topic == topic_list[0]){
		cnt_check ++;
		//Temp = message["Temperature"];
		Temp = message;
		console.log("message1 is "+ message);
	}
	else if( topic == topic_list[1]){
		cnt_check ++;
		//Hum = message["Humidity"];
		Hum = message;
		console.log("message2 is "+ message);
	}
	else if( topic == topic_list[2]){
		cnt_check ++;
		//Gas = message["Gas"];
		Gas = message;
		console.log("message3 is "+ message);
	}
	if( cnt_check == 3 ){
		cnt_check = 0;
		console.log(Temp,Hum,Gas);

		console.log("ready to save");
		var first_name = "DHT-11";
		var Date_and_Time = getDateTime() ; 
		let query = "INSERT INTO sensors (Sensor_ID,Date_and_Time,Temperature,Humidity,Gas) VALUES ('" +
		    first_name + "', '" + Date_and_Time + "', '" + Temp + "', '" + Hum + "','" + Gas + "')";
			connection.query(query, (err, result) => {
		    if (err) {
		        throw err;
		    }
		});

    	var today = new Date();
    	io.sockets.emit(first_name, { time:Date_and_Time , temp:Temp,hum:Hum,gas:Gas}); 
		

		 connection.query("SELECT * FROM sensors ORDER BY id DESC LIMIT 1", function (err, result, fields) {
		 if (err) throw err;
		 result.forEach(function(value) {
		 var m_time = getDateTime();
   		 console.log('temp', { time:m_time , temp:value.Temperature,hum:value.Humidity,gas:value.Gas}); 
   		 io.sockets.emit('temp', { time:m_time , temp:value.Temperature,hum:value.Humidity,gas:value.Gas}); 
	 });
	 
	});
	}

});

client.on("connect",function(){	
	console.log("connected  "+ client.connected);
});

//handle errors
client.on("error",function(error){
console.log("Can't connect" + error);
process.exit(1)});

var options={
retain:true,
qos:1};

console.log("subscribing to topics");
client.subscribe(topic_list,{qos:1}); //topic list
console.log("end of script");

