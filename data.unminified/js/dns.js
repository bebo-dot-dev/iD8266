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
		url : 'getDnsSettings',
		dataType : 'json',
		timeout : 2000
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getDnsSettings ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});
	
	function setDnsPnlState(dnsEnabled) {
		if(dnsEnabled) 
			$('#dnsPnl').fadeIn();
		else
			$('#dnsPnl').fadeOut();
	}

	function initState(dns) {

		$("#hostName").val(dns.hostName);
		dns.mdnsEnabled == "true" ? $("#mdnsEnabled").prop('checked', true) : $("#mdnsEnabled").prop('checked', false);
		dns.dnsEnabled == "true" ? $("#dnsEnabled").prop('checked', true) : $("#dnsEnabled").prop('checked', false);
		setDnsPnlState(dns.dnsEnabled == "true");
		$("#dnsTTL").val(dns.dnsTTL);
		$("#dnsPort").val(dns.dnsPort);
		dns.dnsCatchAll == "true" ? $("#dnsCatchAll").prop('checked', true) : $("#dnsCatchAll").prop('checked', false);
		
		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};

	function checkValid() {
		var valid = true;
		
		function ipValid(elemId, valId, minValue, maxValue) {
			var val = true;
			if (
				($(elemId).val().length == 0) || 
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
		
		valid &= ipValid('#hostName', '#hostNameValFail');
		valid &= ipValid('#dnsTTL', '#dnsTTLValFail', 1, 65535);
		valid &= ipValid('#dnsPort', '#dnsPortValFail', 1, 65535);

		return valid;
	};

	$(document).on('change', 'input[type=checkbox]', function() {
		if ($(this).attr('Id') == 'dnsEnabled')
			setDnsPnlState($("#dnsEnabled").prop('checked'));
		$('button[type=submit]').show();
	});
	$(document).on('keydown', 'input', function() {
		$('button[type=submit]').show();
	});
	$(document).on('click', 'button[type=submit]', function(e) {
		if (!checkValid()) {
			e.preventDefault();
		}
	});
});
