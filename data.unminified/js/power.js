$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();
	
	$.ajax({
		url : 'getPowerMgmtData',
		dataType : 'json'
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = '/login.cm';
		} else {
			console.log('getPowerMgmtData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});
	
	function initState(r) {

		r.enabled == 'true' ? $('#enabled').prop('checked', true) : $('#enabled').prop('checked', false);
		$('#onLength').val(r.onLength);
		$('#offLength').val(r.offLength);
											
		checkSectionDisplay();
		buildScheduleDisplay(r.schedule);		
		
		$('#loading').hide();
		$('#mainPnl').fadeIn();
	};
	
	$(document).on('change', '#enabled, #powerFrm select', function() {
		checkSectionDisplay();
		if ($('#enabled').prop('checked')) {
			$('#generalParamSection').collapse('show');
		}		
		$('#savePwrMgmtSettings').show();
	});
	
	function checkSectionDisplay() {
		if ($('#enabled').prop('checked'))
			$('.pwrMgmtPnl').fadeIn();
		else
			$('.pwrMgmtPnl').hide();				
	}	
	
	function buildScheduleDisplay(schedules) {
	
		if (schedules.length < 14) {
			$('#newScheduleSection').show();
		} else {
			$('#newScheduleSection').hide();
		}
		
		$('#scheduleTable > tbody').html('');
				
		var tRow, tCell, day, hour, offLength, delBtn;
		
		$.each(schedules, function(idx) {
			
			day = $('#scheduleWeekday option[value=' + schedules[idx].weekday + ']').text();
			hour = $('#scheduleHour option[value=' + schedules[idx].hour + ']').text();
			offLength = $('#scheduleOffLength option[value=' + schedules[idx].offLength + ']').text();
			delBtn = '<button type="button" class="btn btn-info btn-sm" data-idx="' +  schedules[idx].Idx  + '"><span class="glyphicon glyphicon-del"></span> Delete</button>';
			
			tRow = $('<tr>');
			
			tCell = $('<td>').html(day);			
			tRow.append(tCell);
			tCell = $('<td>').html(hour);			
			tRow.append(tCell);
			tCell = $('<td>').html(offLength);			
			tRow.append(tCell);
			tCell = $('<td>').html(delBtn);			
			tRow.append(tCell);
			
			$('#scheduleTable').append(tRow);
			
	    });
	    
	}
		
	function saveFeedback(id) {
		$(id).fadeIn('fast', function() {
			window.setTimeout(function() {
				$(id).fadeOut(2000);
			}, 2000);			
		});
	}

	$(document).on('focus', '.apiKey', function() {
		$(this)[0].type = 'text';
	});
	$(document).on('blur', '.apiKey', function() {
		$(this)[0].type = 'password';
	});
	
	$(document).on('keydown', '#powerFrm input', function() {
		$('#savePwrMgmtSettings').show();
	});	
	$(document).on('change', 'input[type=checkbox]', function() {
		$('#savePwrMgmtSettings').show();
	});
		
	$(document).on('click', '#savePwrMgmtSettings', function(e) {				
		$('#savePwrMgmtSettings').hide();
		var postData = $('#powerFrm').serialize(); 
		$.ajax({
			url : 'powerMgmt',
			dataType : 'json',
			data : postData,
			method : 'POST'
		}).done(function(data, textStatus, jqXHR) {
			saveFeedback('#generalSaveSuccess');
		}).fail(function(jqXHR, textStatus, errorThrown) {
			if (jqXHR.status == '403') {
				window.location = '/login.cm';
			} else {
				console.log('powerMgmt ajax call failure', jqXHR, textStatus, errorThrown);
			}
		});		
	});
	
	$(document).on('click', '#saveSchedule', function(e) {		
		
		var postData = 
			'weekday=' + $('#scheduleWeekday').val() + '&' + 
			'hour=' + $('#scheduleHour').val() + '&' + 
			'offLength=' + $('#scheduleOffLength').val();
		 
		$.ajax({
			url : 'addPowerMgmtSchedule',
			dataType : 'json',
			data : postData,
			method : 'POST'
		}).done(function(data, textStatus, jqXHR) {
			$('#newSchedule').collapse('hide');
			saveFeedback('#scheduleSaveSuccess');			
			buildScheduleDisplay(data.schedule);
		}).fail(function(jqXHR, textStatus, errorThrown) {
			if (jqXHR.status == '403') {
				window.location = '/login.cm';
			} else {
				console.log('addPowerMgmtSchedule ajax call failure', jqXHR, textStatus, errorThrown);
			}
		});	
						
	});
		
	$(document).on('click', '#scheduleTable tr button', function(e) {
	
		var postData = 'idx=' + $(e.currentTarget).data('idx');
		$.ajax({
			url : 'removePowerMgmtSchedule',
			dataType : 'json',
			data : postData,
			method : 'POST'
		}).done(function(data, textStatus, jqXHR) {
			saveFeedback('#scheduleSaveSuccess');
			buildScheduleDisplay(data.schedule);
		}).fail(function(jqXHR, textStatus, errorThrown) {
			if (jqXHR.status == '403') {
				window.location = '/login.cm';
			} else {
				console.log('removePowerMgmtSchedule ajax call failure', jqXHR, textStatus, errorThrown);
			}
		});
				
	});		
});