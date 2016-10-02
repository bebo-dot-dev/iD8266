$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();

	$('.valtxt').mask('AAAAAAAABBBBBBBBBBBBBBBBBBBBBBBB', {
		translation : {
			'A' : {
				pattern : /[A-Za-z0-9_.-]/
			},
			'B' : {
				pattern : /[A-Za-z0-9_.-]/,
				optional : true
			}
		}
	});

	$('.number').mask('ABBBBBBB', {
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
		url : 'getLoggingData',
		dataType : 'json'
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = '/login.cm';
		} else {
			console.log('getLoggingData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});

	function initDigiState(pre, d) {
		$(pre + 'NameTitle').text(d.name);
		d.logToThingSpeak == 'true' ? $(pre + 'LogToThingSpeak').prop('checked', true) : $(pre + 'LogToThingSpeak').prop('checked', false);
		d.logToThingSpeak == 'true' ? $(pre + 'ThingSpeakParams').show() : $(pre + 'ThingSpeakParams').hide();
		if (d.thingSpeakChannel != '0') {
			$(pre + 'ThingSpeakChannel').val(d.thingSpeakChannel);
			var url = 'https://thingspeak.com/channels/' + d.thingSpeakChannel;
			$(pre + 'ThingSpeakApiKey').parent().parent().after('<a href="' + url + '" target="_blank">' + url + '</a>');
		}
		$(pre + 'ThingSpeakApiKey').val(d.thingSpeakApiKey);
		d.httpLoggingEnabled == 'true' ? $(pre + 'HttpLoggingEnabled').prop('checked', true) : $(pre + 'HttpLoggingEnabled').prop('checked', false);
	};

	function initState(r) {

		r.thingSpeakEnabled == 'true' ? $('#thingSpeakEnabled').prop('checked', true) : $('#thingSpeakEnabled').prop('checked', false);
		r.httpLoggingEnabled == 'true' ? $('#httpLoggingEnabled').prop('checked', true) : $('#httpLoggingEnabled').prop('checked', false);
		
		$('#httpLoggingHost').val(r.httpLoggingHost);
		$('#httpLoggingUri').val(r.httpLoggingUri);		

		for (var i = 0; i < r.gpio.length; i++) {
			var pre = '#d' + i;
			initDigiState(pre, r.gpio[i]);
		}

		$('#a0NameTitle').text(r.analogName);
		r.analogLogToThingSpeak == 'true' ? $('#a0LogToThingSpeak').prop('checked', true) : $('#a0LogToThingSpeak').prop('checked', false);
		r.analogLogToThingSpeak == 'true' ? $('#a0ThingSpeakParams').show() : $('#a0ThingSpeakParams').hide();
		
		if (r.analogThingSpeakChannel != '0') {
			$('#a0ThingSpeakChannel').val(r.analogThingSpeakChannel);			
			var url = 'https://thingspeak.com/channels/' + r.analogThingSpeakChannel;
			$('#a0ThingSpeakApiKey').parent().parent().after('<a href="' + url + '" target="_blank">' + url + '</a>');	
		}
		$('#a0ThingSpeakApiKey').val(r.analogThingSpeakApiKey);
		r.analogHttpLoggingEnabled == 'true' ? $('#a0HttpLoggingEnabled').prop('checked', true) : $('#a0HttpLoggingEnabled').prop('checked', false);
		
		checkSectionDisplay();
		
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

		if ($('#thingSpeakEnabled').prop('checked')) {
			
			$.each($('.thingspeak input[type=checkbox]'), function(idx, ip) {
			
				var secValid = true;
				
				var pre = ip.id.substring(0, 2);
				var id = '#' + pre + 'LogToThingSpeak';
				if ($(id).prop('checked')) {
					id = '#' + pre + 'ThingSpeakChannel';
					secValid &= ipValid(id, id + 'ValFail');
					
					id = '#' + pre + 'ThingSpeakApiKey';
					secValid &= ipValid(id, id + 'ValFail');
					
					if (!secValid)
						$('#' + pre + 'EditPnl').collapse('show');
		
					valid &= secValid;
				}
				if (idx == 5)
					return false;
			});					
		}
		
		if ($('#httpLoggingEnabled').prop('checked')) {
			
			valid &= ipValid('#httpLoggingHost', '#httpLoggingHostValFail');
			
		}

		return valid;
	};

	function checkSectionDisplay() {
		if ($('#thingSpeakEnabled').prop('checked') || $('#httpLoggingEnabled').prop('checked'))
			$('#ioSection').fadeIn();
		else
			$('#ioSection').fadeOut();
			
		checkThingSpeakDisplay();
		checkHttpLoggingDisplay();
	}
	function checkThingSpeakDisplay() {
		if ($('#thingSpeakEnabled').prop('checked'))
			$('.thingspeak').fadeIn();
		else
			$('.thingspeak').fadeOut();
	}
	function checkHttpLoggingDisplay() {
		if ($('#httpLoggingEnabled').prop('checked'))
			$('.httplogging').fadeIn();
		else
			$('.httplogging').fadeOut();
	}

	$(document).on('focus', '.apiKey', function() {
		$(this)[0].type = 'text';
	});
	$(document).on('blur', '.apiKey', function() {
		$(this)[0].type = 'password';
	});
	
	$(document).on('keydown', 'input', function() {
		$('button[type=submit]').show();
	});
	$(document).on('change', '#thingSpeakEnabled, #httpLoggingEnabled', function() {
		checkSectionDisplay();
		$('button[type=submit]').show();
	});
	$(document).on('change', 'input[type=checkbox]', function() {
		$('button[type=submit]').show();
	});
	$(document).on('change', '.thingspeak input[type=checkbox]', function() {
		var pre = '#' + $(this)[0].id.substring(0, 2);
		var paramSection = $(pre + 'ThingSpeakParams');
		$(this).prop('checked') ? paramSection.fadeIn() : paramSection.fadeOut();
	});
	$(document).on('click', 'button[type=submit]', function(e) {
		if (!checkValid()) {
			e.preventDefault();
		}
	});
	$(document).on('submit', 'form', function(e) {
		if (!checkValid()) {
			e.preventDefault();
		}
	});
}); 
