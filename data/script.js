<script>
if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message:", e.data);
  }, false);
  
  source.addEventListener('staticpower', function(e) {
    console.log("staticpower:", e.data);
    document.getElementById('static_power').innerHTML = e.data;
  }, false);

  source.addEventListener('mqttroot', function(e) {
    console.log("mqttroot:", e.data);
    document.getElementById('mqtt_root').innerHTML = e.data;
  }, false);

  source.addEventListener('mqttstate', function(e) {
    console.log("mqttstate:", e.data);
    document.getElementById('mqtt_state').innerHTML = e.data;
  }, false);

  source.addEventListener('wifirssi', function(e) {
    console.log("wifirssi:", e.data);
    document.getElementById('wifi_rssi').innerHTML = e.data;
  }, false);

  source.addEventListener('clientid', function(e) {
    console.log("clientid:", e.data);
    document.getElementById('client_id').innerHTML = e.data;
  }, false);

  source.addEventListener('uptime', function(e) {
    console.log("uptime:", e.data);
    document.getElementById('up_time').innerHTML = e.data;
  }, false);

  source.addEventListener('currenttime', function(e) {
    console.log("currenttime:", e.data);
    document.getElementById('current_time').innerHTML = e.data;
  }, false);

  source.addEventListener('soyotext', function(e) {
    console.log("soyotext:", e.data);
    document.getElementById('soyo_text').innerHTML = e.data;
  }, false);

  source.addEventListener('checkbox1', function(e) {
    console.log("checkbox1:", e.data);
    document.getElementById('cb1').innerHTML = e.data;
  }, false);

  source.addEventListener('checkbox2', function(e) {
    console.log("checkbox2:", e.data);
    document.getElementById('cb2').innerHTML = e.data;
  }, false);

  source.addEventListener('checkbox3', function(e) {
    console.log("checkbox3:", e.data);
    document.getElementById('cb3').innerHTML = e.data;
  }, false);
  
  source.addEventListener('time1', function(e) {
    console.log("time1:", e.data);
    document.getElementById('t1').innerHTML = e.data;
  }, false);

source.addEventListener('watt1', function(e) {
    console.log("watt1:", e.data);
    document.getElementById('w1').value = parseInt(e.data);
  }, false);

  source.addEventListener('watt2', function(e) {
    console.log("watt2:", e.data);
    document.getElementById('w2').value = parseInt(e.data);
  }, false);
}

function set_power(value) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", value, true);
  xhr.send();
};

function balance_toggle() {
  let text = "Toggle BMS Autobalance?";
  if (confirm(text) == true) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/balance.toggle", true);
    xhr.send();
  } else {
    text = " canceled!";
  }
};

function restart() {
  let text = "Restart System?";
  if (confirm(text) == true) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/restart", true);
    xhr.send();
  } else {
    text = " canceled!";
  } 
};

function savetime() {
  var time1 = document.getElementById("t1").value;
  var watt1 = document.getElementById("w1").value;
  
  let text = "Save Settings!\nPress OK or Cancel.";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/savetime?t1=" + time1 +"&w1=" + watt1, true);
  xhr.send(); 
};

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/checkbox?cbid="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/checkbox?cbid="+element.id+"&state=0", true); }
  xhr.send();
}

function uhrzeit() {
  var jetzt = new Date();
  h = jetzt.getHours();
  m = jetzt.getMinutes();
  s = jetzt.getSeconds();
  h = fNull(h);
  m = fNull(m);
  s = fNull(s);
  document.getElementById('uhr').innerHTML = h + ':' + m + ':' + s;
  setTimeout(uhrzeit, 5000);
};

var mydate = new Date();
var options = {year: 'numeric', month: '2-digit', day: '2-digit'};
document.getElementById("datum").textContent = mydate.toLocaleString('de-DE', options);

function fNull(zahl) {
  zahl = (zahl < 10 ? '0' : '' ) + zahl;  
  return zahl;
};

</script>