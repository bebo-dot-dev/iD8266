$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();

	$('.number').mask('AAAAAA', {
		translation : {
			'A' : {
				pattern : /[0-9]/
			}
		},
		clearIfNotMatch : true
	});
		
	$.ajax({
		url : 'getSecurityData',
		dataType : 'json',
		timeout : 2000
	}).done(function(d, textStatus, jqXHR) {
		initState(d);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getSecurityData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});

	function initState(r) {

		if (r.ntpEnabled == 'true') {
			$('#totpPnl').show();
		}

		r.totpEnabled == 'true' ? $('#totpEnabled').prop('checked', true) : $('#totpEnabled').prop('checked', false);
		if (r.totpEnabled == 'true') {
			$('#totpStatus').removeClass('alert-danger').addClass('alert-success');
			$('#totpStatus span').text('Activated');
		}

		$('#pleaseWait').hide();
		$('#mainPnl').fadeIn();
	};

	var pwdValid;

	$('#pwd').complexify({}, function(valid, complexity) {
		pwdValid = valid;
		var progressBar = $('#complexity-bar');

		progressBar.toggleClass('progress-bar-success', valid);
		progressBar.toggleClass('progress-bar-danger', !valid);
		progressBar.css({
			'width' : complexity + '%'
		});

		$('.complexity').text(Math.round(complexity) + '%');

		checkMatch();
	});

	$('#confirmPwd').on('change keyup focus mouseup', function() {
		checkMatch();
	});
	
	function enableDisableBtn(selector, enabled) {
		if (enabled) {
			$(selector).removeClass('disabled');
			$(selector).prop('disabled', false);
		} else {
			if (!$(selector).hasClass('disabled')) {
				$(selector).addClass('disabled');
			}
			$(selector).addClass('disabled');
			$(selector).prop('disabled', true);
		}
	}

	function checkMatch() {
		if (pwdValid && $('#pwd').val() == $('#confirmPwd').val()) {
			enableDisableBtn('#passwordFrm button', true);			
		} else {
			enableDisableBtn('#passwordFrm button', false);
		}
	}


	$(document).on('change', '#totpEnabled', function() {

		if ($('#totpEnabled').prop('checked')) {
			
			$('#totpFrm button').addClass('disabled');
			$('#totpFrm button').prop('disabled', true);
			
			//get a new totp qr code and display it for confirmation
			$.ajax({
				url : 'getTotpQrCodeData',
				dataType : 'json',
				timeout : 2000
			}).done(function(d, textStatus, jqXHR) {
				
				var context = $('#qrCode')[0].getContext('2d');
				context.clearRect(0, 0, 225, 225);
				$('#qrCode').qrcode({ecLevel: 'H', size: 225, text: d.totpQrCodeUri});			
				$('#totp').val('');

				enableDisableBtn('#totpFrm button', false);
				
				$('#totpStatus').removeClass('alert-success').addClass('alert-danger');
				$('#totpStatus span').text('Deactivated');

				$('#totpConfirm').fadeIn();

			}).fail(function(jqXHR, textStatus, errorThrown) {
				if (jqXHR.status == '403') {
					window.location = "/login.cm";
				} else {
					console.log('getTotpQrCodeData ajax call failure', jqXHR, textStatus, errorThrown);
				}
			});
		} else {
			$('#totpConfirm').fadeOut();
			enableDisableBtn('#totpFrm button', true);			
		}
	});

	$('#totp').on('change keyup', function() {
		var totp = $('#totp').val();
		if (totp.length == 6) {
			enableDisableBtn('#totpFrm button', true);
		} else {
			enableDisableBtn('#totpFrm button', false);
		}
	});
	
	$(document).on('click', '#totpFrm button', function(e) {		
		
		if ($('#totpEnabled').prop('checked')) {
			
			//switching on totp
				
			var postData = 'totp=' + $('#totp').val();
		 
			$.ajax({
				url : 'submitTotpToken',
				dataType : 'json',
				data : postData,
				method : 'POST'
			}).done(function(d, textStatus, jqXHR) {
				if (d.totpEnabled == 'true') {
					$('#totpStatus').removeClass('alert-danger').addClass('alert-success');
					$('#totpStatus span').text('Activated');
					$('#totpConfirm').fadeOut();
					enableDisableBtn('#totpFrm button', false);
				} else {
					$('#totpStatus span').text('Try Again');					
				}
			}).fail(function(jqXHR, textStatus, errorThrown) {
				if (jqXHR.status == '403') {
					window.location = '/login.cm';
				} else {
					console.log('submitTotpToken ajax call failure', jqXHR, textStatus, errorThrown);
				}
			});
				
		} else {
			
			//switching off totp
			
			var postData = 'totpOff=1';
			
			//get a new totp qr code and display it for confirmation
			$.ajax({
				url : 'getTotpQrCodeData',
				dataType : 'json',
				data : postData,
				timeout : 2000
			}).done(function(d, textStatus, jqXHR) {

				$('#totpStatus').removeClass('alert-success').addClass('alert-danger');
				$('#totpStatus span').text('Deactivated');
				enableDisableBtn('#totpFrm button', false);

			}).fail(function(jqXHR, textStatus, errorThrown) {
				if (jqXHR.status == '403') {
					window.location = "/login.cm";
				} else {
					console.log('getTotpQrCodeData ajax call failure', jqXHR, textStatus, errorThrown);
				}
			});
		}							
	});
});
