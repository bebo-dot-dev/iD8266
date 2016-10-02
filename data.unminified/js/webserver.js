$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();

	$('.valtxt').mask('AAAAAAAABBBBBBBBBBBBBBBBBBBBBBBB', {
		translation : {
			'A' : {
				pattern : /[A-Za-z0-9_]/
			},
			'B' : {
				pattern : /[A-Za-z0-9_]/,
				optional : true
			}
		}
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
		url : 'getWebserverSettings',
		dataType : 'json',
		timeout : 2000
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = '/login.cm';
		} else {
			console.log('getWebserverSettings ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});

	function initState(server) {

		$("#adminPwd").val(server.adminPwd);
		$("#webserverPort").val(server.webserverPort);
		$("#websocketServerPort").val(server.websocketServerPort);
		server.includeServerHeader == "true" ? $("#includeServerHeader").prop('checked', true) : $("#includeServerHeader").prop('checked', false);
		$("#serverHeader").val(server.serverHeader);
		
		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};

	function checkValid() {
		var valid = true;
		
		function ipValid(elemId, valId, minLength, minValue, maxValue) {
			var val = true;
			if (
				($(elemId).val().length == 0) || 
				((minLength) && $(elemId).val().length < minLength) ||
				((minValue) && parseInt($(elemId).val()) < minValue) ||
				((maxValue) && parseInt($(elemId).val()) > maxValue)
				) {
				val = false;
				$(valId).css('display', 'block');
			} else {
				$(valId).css('display', 'none');
			}
			return val;
		}
		
		valid &= ipValid('#adminPwd', '#adminPwdValFail', 8);
		valid &= ipValid('#webserverPort', '#webserverPortValFail', null, 1, 65535);
		valid &= ipValid('#websocketServerPort', '#websocketServerPortValFail', null, 1, 65535);
		valid &= ipValid('#serverHeader', '#serverHeaderValFail', 8);

		return valid;
	};

	$(document).on('change', 'input[type=checkbox]', function() {
		$('button[type=submit]').show();
	});
	$(document).on('keydown', 'input', function() {
		$('button[type=submit]').show();
	});
	$(document).on('focus', '#adminPwd', function() {
		$(this)[0].type = 'text';
	});
	$(document).on('blur', '#adminPwd', function() {
		$(this)[0].type = 'password';
	});
	$(document).on('click', 'button[type=submit]', function(e) {
		if (!checkValid()) {
			e.preventDefault();
		}
	});
});
