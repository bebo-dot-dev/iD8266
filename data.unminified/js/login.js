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
		
		if (d.totpEnabled == 'true') {
			$('#totpPnl').fadeIn(400, function() {
		    	$('#totpPnl').removeClass('dnone');
  			});
		}
		
	}).fail(function(jqXHR, textStatus, errorThrown) {
		window.location = "/login.cm";
		console.log('getSecurityData ajax call failure', jqXHR, textStatus, errorThrown);
	});
	
	$(document).on('click', '#loginFrm button[type=submit]', function(e) {
	
		function ipValid(elemId, valId, pnl, minLength) {
			var val = true;
			if (($(elemId).val().length == 0) || ((minLength) && $(elemId).val().length < minLength)) {
				val = false;
				$(valId).css('display', 'block');
				$(pnl).addClass('has-error');
			} else {
				$(valId).css('display', 'none');
				$(pnl).removeClass('has-error');
			}
			return val;
		}
		
		var valid = ipValid('#pwdIp', '#pwdValFail', '#pwd', 8);
		if (!$('#totpPnl').hasClass('dnone')) {
			valid &= ipValid('#totp', '#otpValFail', '#totpPnl', 6);
		}
		
		if(!valid) {
			e.preventDefault();
		}
			
	});	
});