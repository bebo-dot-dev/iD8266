$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	var d;
	var socket;
	
	var rangeSlider = function() {
		var slider = $('.range-slider'),
		    range = $('.range-slider__range'),
		    value = $('.range-slider__value');
		slider.each(function() {
			value.each(function() {
				var value = $(this).prev().attr('value');
				$(this).html(value);
			});
			range.on('input', function() {
				$(this).next(value).html(this.value);
			});
		});
	};
	rangeSlider();

	$.ajax({
		url : 'getGpioData' + window.location.search,
		dataType : 'json'		
	}).done(function(data, textStatus, jqXHR) {
		d = data;
		initState(data);
		connectSocket(d.webserver);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getGpioData ajax call failure', jqXHR, textStatus, errorThrown);			
		}
	});

	function initState(r) {

		var isInput = false;
		var valid = true;
		if (r.Mode) {
			//digital
			$('#dNameTitle').text(r.Name);
			switch(r.Mode) {
			case '1':
			case '2':
				isInput = true;
				$('#dMode').text(r.Mode == '1' ? '(Digital Input)' : '(Digital Input with Pullup)');
				$('.range-slider').hide();
				$('#switch').attr('disabled', 'disabled');
				$('.onoffswitch').show();
				r.Value == '0' ? $('#switch').prop('checked', false) : $('#switch').prop('checked', true);
				break;
			case '3':
				$('#dMode').text('(Digital Output)');
				$('.range-slider').hide();
				$('#switch').removeAttr('disabled');
				$('.onoffswitch').show();
				r.Value == '0' ? $('#switch').prop('checked', false) : $('#switch').prop('checked', true);
				break;
			case '4':
				$('#dMode').text('(PWM Analog Output)');
				$('.onoffswitch').hide();
				$('.range-slider').show();
				$('.range-slider__range').val(r.Value);
				$('.range-slider_value').text(r.Value);
				break;
			default:
				valid = false;
				$('#analogPanel').hide();
				$('#digitalPanel').hide();
				break;
			}
			if (valid) {
				$('#analogPanel').hide();
				$('#digitalPanel').show();
			} else
				$('#mainPnl .panel-body').text(r.Name + ' is configured as unused');

		} else {
			//analog
			$('#aNameTitle').text(r.A0Name);
			if (r.A0Mode == '1') {
				isInput = true;
				$('#a0Calculated').text(r.A0Calculated);
				$('#a0UnitStr').text(r.A0UnitStr);
				$('#a0Value').text(r.A0Value);
				$('#a0Voltage').text(r.A0Voltage);
				$('#digitalPanel').hide();
				$('#analogPanel').show();
			} else {
				valid = false;
				$('#mainPnl .panel-body').text(r.A0Name + ' is configured as unused');
			}
		}

		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};
	
	function connectSocket(ws) {
		
		var scheme = ws.sslProxyEnabled == 'true' ? 'wss://' : 'ws://';
		var port = ws.sslProxyWebsocketPort ? ws.sslProxyWebsocketPort : ws.websocketServerPort;
		var socketUrl = scheme + window.location.hostname + ':' + port;
		try
		{
			var socket = new WebSocket(socketUrl, ['arduino']);
		}
		catch(e)
		{
			console.log('WebSocket Error ', e);
			$('.socketStatus').removeClass('connected').addClass('disconnected');
			$('.statusTxt').text('websocket error to ' + window.location.hostname + ' on port ' + port + '. Check firewall configuration and network stability.');
			$('.statusTxt').show();
		}
		
		socket.onopen = function () {
			$('.socketStatus').addClass('connected');
			
			$(window).on('offline online', function (e) {
				console.log('Offline/Online ', e);
			    if (e.type == 'offline') {
			    	socketClosed();
			    } else {
			    	window.location.reload(true);
			    }
			});
		};
		
		socket.onerror = function (error) {
			console.log('WebSocket Error ', error);
			$('.socketStatus').removeClass('connected').addClass('disconnected');
			var port = ws.sslProxyWebsocketPort ? ws.sslProxyWebsocketPort : ws.websocketServerPort;
			$('.statusTxt').text('websocket error to ' + window.location.hostname + ' on port ' + port + '. Check firewall configuration and network stability.');
			$('.statusTxt').show();
		};
		
		socket.onclose = function (e) {
			console.log('WebSocket Closed ', e);
			socketClosed();
		};
		
		function socketClosed() {
			$('.socketStatus').removeClass('connected').addClass('disconnected');
			var port = ws.sslProxyWebsocketPort ? ws.sslProxyWebsocketPort : ws.websocketServerPort;
			$('.statusTxt').text('websocket connection to ' + window.location.hostname + ' on port ' + port + ' closed.');
			$('.statusTxt').show();
		}
		
		socket.onmessage = function (e) {
			console.log('Server: ', e.data);
			var j = JSON.parse(e.data);
			if(d.Mode && j.GPIOType == '0' && j.Idx == d.Idx) {
				//digital
				switch(d.Mode) {
				case '1':
				case '2':
				case '3':
					j.Value == '0' ? $('#switch').prop('checked', false) : $('#switch').prop('checked', true);
					break;
				case '3':
					$('.range-slider__range').val(j.Value);
					$('.range-slider_value').text(j.Value);
					break;
				}
			} else {
				//analog				
				$('#a0Value').text(j.Value);
				$('#a0Voltage').text(j.Voltage);
				$('#a0Calculated').text(j.Calculated);
			}
		};
	};
	
	$(document).on('change', '#switch', function() {
		if ((d) && (d.Mode) && (d.Mode == '3')) {
			var bitVal = $(this).prop('checked') ? '1' : '0';
			var qs = '?d=' + d.Idx + '&value=' + bitVal;
			$.ajax({
				url : '/digitalWrite' + qs,
				dataType : 'json',
				timeout : 5000
			}).fail(function(jqXHR, textStatus, errorThrown) {
				if (jqXHR.status == '403') {
					window.location = "/logon";
				} else {
					console.log('digitalWrite ajax call failure', jqXHR, textStatus, errorThrown);					
				}
			});
		}
	});

	$(document).on('change', '#range', function() {
		if ((d) && (d.Mode) && (d.Mode == '4')) {
			var analogVal = $(this).val();
			var qs = '?d=' + d.Idx + '&value=' + analogVal;
			$.ajax({
				url : '/analogWrite' + qs,
				dataType : 'json',
				timeout : 5000
			}).fail(function(jqXHR, textStatus, errorThrown) {
				if (jqXHR.status == '403') {
					window.location = "/logon";
				} else {
					console.log('analogWrite ajax call failure', jqXHR, textStatus, errorThrown);
				}
			});
		}
	});
}); 
