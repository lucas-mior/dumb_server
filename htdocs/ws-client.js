var line1 = new TimeSeries();
var line2 = new TimeSeries();

setInterval(function() {
    let date = Date.now();
    console.log("Date: ", date);
    line1.append(date, Math.random());
    line2.append(date, Math.random());
}, 1000);

var smoothie = new SmoothieChart({
    grid: { 
        strokeStyle: 'rgb(125, 0, 0)',
        fillStyle: 'rgb(60, 0, 0)',
        lineWidth: 1,
        millisPerLine: 250,
        verticalSections: 6 
    }
});

smoothie.addTimeSeries(
    line1,
    { 
        strokeStyle: 'rgb(0, 255, 0)',
        fillStyle: 'rgba(0, 255, 0, 0.4)',
        lineWidth: 3 
    }
);
smoothie.addTimeSeries(
    line2,
    { 
        strokeStyle: 'rgb(255, 0, 255)',
        fillStyle: 'rgba(255, 0, 255, 0.3)',
        lineWidth: 3 
    }
);

smoothie.streamTo(document.getElementById("#plotcanvas"), 1000);

function setUpCanvas() {
    canvas = document.getElementsByClassName("#plotcanvas")[0];
    ctx = canvas.getContext('2d');
    ctx.translate(0.5, 0.5);

    // Set display size (vw/vh).
    var sizeWidth = 80 * window.innerWidth / 100,
      sizeHeight = 80 * window.innerHeight / 100 || 766;

    //Setting the canvas site and width to be responsive 
    canvas.width = sizeWidth;
    canvas.height = sizeHeight;
    canvas.style.width = sizeWidth;
    canvas.style.height = sizeHeight;
}

window.addEventListener('onload', setUpCanvas, false);
window.addEventListener('resize', setUpCanvas, false);

/**
 * client code
 * Runs in browser
 */
;(function () {
	"use strict";

	// A couple helpers to get elements by ID:
	function qs(sel) { return document.querySelector(sel); }
	function qsa(sel) { return document.querySelectorAll(sel); }

	let ws; // the websocket

	function parseLocation(url) {
		let a = document.createElement('a');
		a.href = url;

		return a;
	}

	function escapeHTML(s) {
		return s.replace(/&/g, '&amp;')
			.replace(/</g, '&lt;')
			.replace(/>/g, '&gt;')
			.replace(/'/g, '&apos;')
			.replace(/"/g, '&quot;')
			.replace(/\//g, '&sol;');
	}

	function writeOutput(s) {
		let chatOutput = qs('#chat-output');
		let innerHTML = chatOutput.innerHTML;

		let newOutput = innerHTML === ''? s: '<br/>' + s;
		chatOutput.innerHTML = innerHTML + newOutput;
		chatOutput.scrollTop = chatOutput.scrollHeight;
	}

	function sendMessage(type, payload) {
        let message = JSON.stringify({
			'type': type,
			'payload': payload
		});
		ws.send(message);
	}

	function send() {
		sendMessage('chat-message', {
			"username": getChatUsername(),
			"message": getChatMessage()
		});

		// Clear the input field after sending
		qs('#chat-input').value = '';
	}

	function onChatInputKeyUp(ev) {
		if (ev.keyCode === 13) { // 13 is RETURN
			send();
		}
	}

	function onSocketMessage(ev) {
		let msg = JSON.parse(ev.data);
		let payload = msg.payload;

		let username = escapeHTML(payload.username);

		switch (msg.type) {
			case 'chat-message':
				writeOutput('<b>' + username + ":</b> " + escapeHTML(payload.message));
				break;

			case 'chat-join':
				writeOutput('<i><b>' + username + '</b> has joined the chat.</i>');
				break;

			case 'chat-leave':
				writeOutput('<i><b>' + username + '</b> has left the chat.</i>');
				break;
		}
	}

	function onLoad() {
		let localURL = parseLocation(window.location);

		qs('#chat-input').addEventListener('keyup', onChatInputKeyUp);
		qs('#chat-send').addEventListener('click', send);

		// Create WebSocket
		ws = new WebSocket("ws://" + localURL.host, "beej-chat-protocol");

		ws.addEventListener('open', onSocketOpen);
		ws.addEventListener('close', onSocketClose);
		ws.addEventListener('error', onSocketError);
		ws.addEventListener('message', onSocketMessage);

		let userName = getChatUsername().trim();

		if (userName === '') {
			qs('#chat-username').value = 'Guest ' +
				((Math.random()*0xffff)|0).toString(16);
		}
	}

	// Wait for load event before starting
	window.addEventListener('load', onLoad);
}());
