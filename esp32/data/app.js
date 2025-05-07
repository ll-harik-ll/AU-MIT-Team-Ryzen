const socket = new WebSocket('ws://' + location.hostname + ':81');

socket.onmessage = function(event) {
  const [light1, light2] = event.data.split(",");
  document.getElementById("light1").className = "light " + light1;
  document.getElementById("light2").className = "light " + light2;
};
