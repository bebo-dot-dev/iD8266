$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();
	
	$.ajax({
		url : 'getPeripheralsData',
		dataType : 'json'
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = '/login.cm';
		} else {
			console.log('getPeripheralsData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});
	
	function initState(r) {
		
		buildPeripheralDisplay(r.peripheral);		
		
		$('#loading').hide();
		$('#mainPnl').fadeIn();
	};
			
	function buildPeripheralDisplay(peripherals) {
	
		if (peripherals.length < 5) {
			$('#newPeripheralSection').show();
		} else {
			$('#newPeripheralSection').hide();
		}
		
		if (peripherals.length > 0) {
			$('#peripheralTableSection').show();			
		} else {
			$('#peripheralTableSection').hide();
		}
		
		$('#peripheralTable > tbody').html('');
				
		var tRow, tCell, pIdx, type, pin, name, delBtn;
		
		$.each(peripherals, function(idx) {
			
			pIdx = peripherals[idx].Idx;
			type = $('#peripheralType option[value=' + peripherals[idx].peripheralType + ']').text();
			pin = $('#pinIdx option[value=' + peripherals[idx].peripheralPin + ']').text();
			name = peripherals[idx].Name
			delBtn = '<button type="button" class="btn btn-info btn-sm" data-idx="' + pIdx + '"><span class="glyphicon glyphicon-del"></span> Delete</button>';
			
			tRow = $('<tr>');
			
			tCell = $('<td>').html(pIdx);			
			tRow.append(tCell);
			tCell = $('<td>').html(type);			
			tRow.append(tCell);
			tCell = $('<td>').html(pin);			
			tRow.append(tCell);
			tCell = $('<td>').html(name);			
			tRow.append(tCell);
			tCell = $('<td>').html(delBtn);			
			tRow.append(tCell);
			
			$('#peripheralTable').append(tRow);
			
	    });
	    
	}
		
	function saveFeedback(id) {
		$(id).fadeIn('fast', function() {
			window.setTimeout(function() {
				$(id).fadeOut(2000);
			}, 2000);			
		});
	}
	
	function ipValid(elemId, valId, minLength) {
		var val = true;
		if ($(elemId).val().length == 0) {
			val = false;
			$(valId).css('display', 'block');
		} else {
			$(valId).css('display', 'none');
		}
		return val;
	}
	
	$(document).on('click', '#savePeripheral', function(e) {		
		
		if (ipValid('#name', '#nameValFail')) {
					
			var postData = 
				'peripheralType=' + $('#peripheralType').val() + '&' + 
				'Name=' + $('#name').val() + '&' + 
				'idx=' + $('#pinIdx').val()  + '&' + 
			 	'Default=' + $('#default').val();
			 	
			$.ajax({
				url : 'addPeripheral',
				dataType : 'json',
				data : postData,
				method : 'POST'
			}).done(function(data, textStatus, jqXHR) {
				$('#newPeripheral').collapse('hide');
				saveFeedback('#peripheralSaveSuccess');			
				buildPeripheralDisplay(data.peripheral);
			}).fail(function(jqXHR, textStatus, errorThrown) {
				if (jqXHR.status == '403') {
					window.location = '/login.cm';
				} else {
					console.log('addPeripheral ajax call failure', jqXHR, textStatus, errorThrown);
				}
			});
		
		}			
	});
		
	$(document).on('click', '#peripheralTable tr button', function(e) {
	
		var postData = 'idx=' + $(e.currentTarget).data('idx');
		$.ajax({
			url : 'removePeripheral',
			dataType : 'json',
			data : postData,
			method : 'POST'
		}).done(function(data, textStatus, jqXHR) {
			saveFeedback('#peripheralSaveSuccess');
			buildPeripheralDisplay(data.peripheral);
		}).fail(function(jqXHR, textStatus, errorThrown) {
			if (jqXHR.status == '403') {
				window.location = '/login.cm';
			} else {
				console.log('removePeripheral ajax call failure', jqXHR, textStatus, errorThrown);
			}
		});
				
	});		
});