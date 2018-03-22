$(document).ready(function() {
	window.setInterval(function() {
		var sec = window.parseInt($('#cnt').text());
		sec--;
		if (sec < 0) {
			window.location = "/";
		} else {
			$('#cnt').text(sec);
		}
	}, 1000);
}); 