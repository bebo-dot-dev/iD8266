$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();

	$('.valtxt').mask('AAAAAAAABBBBBBBBBBBBBBBBBBBBBBBB', {
		translation : {
			'A' : {
				pattern : /[A-Za-z0-9_.-\s]/
			},
			'B' : {
				pattern : /[A-Za-z0-9_.-\s]/,
				optional : true
			}
		}
	});
	
	$('.ip_address').mask('0ZZ.0ZZ.0ZZ.0ZZ', {
		translation : {
			'Z' : {
				pattern : /[0-9]/,
				optional : true
			}
		},
		clearIfNotMatch : true
	});

	$('.number').mask('ABBBB', {
		translation : {
			'A' : {
				pattern : /[0-9]/
			},
			'B' : {
				pattern : /[0-9]/,
				optional : true
			}
		},
		clearIfNotMatch : true
	});

	$.ajax({
		url : 'getMqttData',
		dataType : 'json',
		timeout : 2000
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getMqttData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});
	
	function initState(r) {

		r.mqttSystemEnabled == "true" ? $("#mqttSystemEnabled").prop('checked', true) : $("#mqttSystemEnabled").prop('checked', false);
		checkMqttDisplay();

		$('#mqttServerBrokerIp').val(r.mqttServerBrokerIp);
		$('#mqttServerBrokerPort').val(r.mqttServerBrokerPort);
		$('#mqttUsername').val(r.mqttUsername);
		$('#mqttPassword').val(r.mqttPassword);
		
		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};

	function checkValid() {

		var valid = true;

		function ipValid(elemId, valId, minValue, maxValue) {
			var val = true;
			if (($(elemId).val().length == 0) || ((minValue) && parseInt($(elemId).val()) < minValue) || ((maxValue) && parseInt($(elemId).val()) > maxValue)) {
				val = false;
				$(valId).css('display', 'block');
			} else {
				$(valId).css('display', 'none');
			}
			return val;
		}
											
		if ($("#mqttSystemEnabled").prop('checked')) {
			var mqttValid = ipValid('#mqttServerBrokerIp', '#mqttServerBrokerIpValFail');
			mqttValid &= ipValid('#mqttServerBrokerPort', '#mqttServerBrokerPortValFail', 1, 65535);
			mqttValid &= ipValid('#mqttUsername', '#mqttUsernameValFail');
			mqttValid &= ipValid('#mqttPassword', '#mqttPasswordValFail');
			
			valid &= mqttValid;
			
			if (!mqttValid)
				$('#mqttEditPnl').collapse('show');
		}

		return valid;
	};

	function checkMqttDisplay() {
		if ($("#mqttSystemEnabled").prop('checked'))
			$('.mqtt').fadeIn();
		else
			$('.mqtt').fadeOut();
	}

	$(document).on('focus', '#mqttPassword', function() {
		$(this)[0].type = 'text';
	});
	$(document).on('blur', '#mqttPassword', function() {
		$(this)[0].type = 'password';
	});
		
	$(document).on('keydown', 'input', function() {
		$('button[type=submit]').show();
	});
	
	$(document).on('change', '#mqttSystemEnabled', function() {
		checkMqttDisplay();
	});
	$(document).on('change', 'input[type=checkbox]', function() {
		$('button[type=submit]').show();
	});
	$(document).on('click', 'button[type=submit]', function(e) {
		if (!checkValid()) {
			e.preventDefault();
		}
	});
}); 
