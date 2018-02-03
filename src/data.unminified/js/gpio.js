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

	$('.d').mask('N', {
		translation : {
			'N' : {
				pattern : /[0-1]/,
				optional : true
			}
		},
		clearIfNotMatch : true
	});

	$('.ad').mask('0ZZ', {
		translation : {
			'Z' : {
				pattern : /[0-9]/,
				optional : true
			}
		},
		clearIfNotMatch : true
	});
		
	$.ajax({
		url : 'getGpioData',
		dataType : 'json',
		timeout : 2000
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getGpioData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});

	function initDigiState(pre, d) {
		$(pre + 'NameTitle').text(d.Name);
		$(pre + 'PinMode').val(d.Mode);
		$(pre + 'Name').val(d.Name);
		$(pre + 'Default').val(d.Default);	
	};

	function initState(r) {
		
		for (var i = 0; i < r.gpio.length; i++) {
			var pre = '#d' + i;
			initDigiState(pre, r.gpio[i]);
		}

		$('#a0NameTitle').text(r.A0Name);
		$('#a0PinMode').val(r.A0Mode);
		$('#a0Name').val(r.A0Name);
		$('#a0Offset').val(r.A0Offset);
		$('#a0Multiplier').val(r.A0Multiplier);
		$('#a0Unit').val(r.A0Unit);
		
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
		
		function floatValid(elemId, valId) {
			var val = true;
			if (($(elemId).val().length == 0) || (isNaN(parseFloat($(elemId).val())))) {
				val = false;
				$(valId).css('display', 'block');
			} else {
				$(valId).css('display', 'none');
			}
			return val;
		}

		$.each($('select'), function(idx, sel) {
			var secValid = true;
			var pre = sel.id.substring(0, 2);
			var d = '#' + pre + 'Default';
			if ($(sel).val() == '3') {
				secValid &= ipValid(d, d + 'ValFail', 0, 255);
			} else {
				secValid &= ipValid(d, d + 'ValFail', 0, 1);
			}
			var n = '#' + pre + 'Name';
			secValid &= ipValid(n, n + 'ValFail');
			if (!secValid)
				$('#' + pre + 'EditPnl').collapse('show');

			valid &= secValid;

			if (idx == 4)
				return false;
		});

		var anValid = ipValid('#a0Name', '#a0NameValFail');
		anValid &= floatValid('#a0Offset', '#a0OffsetValFail');
		anValid &= floatValid('#a0Multiplier', '#a0MultiplierValFail');		
						
		if (!anValid)
			$('#a0EditPnl').collapse('show');
			
		valid &= anValid;
					
		return valid;
	};
	
	$(document).on('change', 'select', function() {

		var pre = $(this)[0].id.substring(0, 2);
		var d = $('#' + pre + 'Default');
		switch($(this).val()) {
		case '1':
		case '2':
			d.removeClass('ad').addClass('d');
			if (d.val() != '0' && d.val() != '1')
				d.val('0');
			break;
		case '3':
			d.removeClass('d').addClass('ad');
			break;
		default:
			break;
		}
		$(this).attr('name', $(this).attr('id'));
		$('button[type=submit]').show();
	});

	$(document).on('blur', '.ad', function() {
		$(this).val('' + parseInt($(this).val()));
	});
	$(document).on('keydown', 'input', function() {
		$(this).attr('name', $(this).attr('id'));
		$('button[type=submit]').show();
	});
	$(document).on('keypress', '.float', function (e) {
        if (e.shiftKey == true) {
            e.preventDefault();
        }

        if ((e.keyCode >= 48 && e.keyCode <= 57) ||             
            e.keyCode == 8 || e.keyCode == 9 || e.keyCode == 13 ||  
            e.keyCode == 45 || e.keyCode == 46 || e.keyCode == 110 || e.keyCode == 190) {

        } else {
            e.preventDefault();
        }
        
        if($(this).val() !== '' && (e.keyCode == 45))
            e.preventDefault();

        if($(this).val().indexOf('.') !== -1 && (e.keyCode == 110 || e.keyCode == 190))
            e.preventDefault();         
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