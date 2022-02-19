var rainbowEnable = false;
var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
connection.onopen = function () {
  connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {
  console.log('Server: ', e.data);
  if(e.data[0] == '#'){
    receiveRGB(e.data.slice(1));
  }
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};

function sendRGB () {
  let r = document.getElementById('r').value** 2 / 1023;
  let g = document.getElementById('g').value** 2 / 1023;
  let b = document.getElementById('b').value** 2 / 1023;

  let rgb = r << 20 | g << 10 | b;
  let rgbstr = '#' + rgb.toString(16);
  console.log('RGB: ' + rgbstr);
  connection.send(rgbstr);
}

function receiveRGB(rgbstr){
  let r, g, b = '';
  
  const binary = (parseInt(rgbstr, 16).toString(2)).padStart(30, '0');
  r = parseInt(binary.slice(0, 10),2);
  g = parseInt(binary.slice(10, 20),2);
  b = parseInt(binary.slice(20, 30),2);

  r = (r*1023)**(1/2);
  g = (g*1023)**(1/2);
  b = (b*1023)**(1/2);
  document.getElementById('r').value = r;
  document.getElementById('g').value = g;
  document.getElementById('b').value = b;
}

function rainbowEffect () {
  rainbowEnable = ! rainbowEnable;
  if (rainbowEnable) {
    connection.send("R");
    document.getElementById('rainbow').style.backgroundColor = '#00878F';
    document.getElementById('r').className = 'disabled';
    document.getElementById('g').className = 'disabled';
    document.getElementById('b').className = 'disabled';
    document.getElementById('r').disabled = true;
    document.getElementById('g').disabled = true;
    document.getElementById('b').disabled = true;
  } else {
    connection.send("N");
    document.getElementById('rainbow').style.backgroundColor = '#999';
    document.getElementById('r').className = 'enabled';
    document.getElementById('g').className = 'enabled';
    document.getElementById('b').className = 'enabled';
    document.getElementById('r').disabled = false;
    document.getElementById('g').disabled = false;
    document.getElementById('b').disabled = false;
    sendRGB();
  }
}