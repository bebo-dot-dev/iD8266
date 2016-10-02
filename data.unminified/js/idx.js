var updateUI = (function update(d) {
		
	var a;
	if(d) {
		localStorage.setItem('deviceClassString', d.runtimeData.deviceClassString);
		localStorage.setItem('firmwareVersion', d.runtimeData.firmwareVersion);
		
		for (var i = 0; i < d.gpioData.gpio.length; i++) {
			var gpio = d.gpioData.gpio[i];
			if (gpio.Mode != '0') {
				localStorage.setItem('d' + i + 'Name', gpio.Name);
			} else
				localStorage.setItem('d' + i + 'Name', 'Unused');
			
			switch(gpio.Mode)
			{
				case '1':
				case '2':
					localStorage.setItem('d' + i + 'Info', 'Monitor "' + gpio.Name + '"');
					break;
				case '3':
				case '4':
					localStorage.setItem('d' + i + 'Info', 'Control "' + gpio.Name + '"');
					break;
				default:
					localStorage.setItem('d' + i + 'Info', 'D' + i + ' is unused');
					break;
			}
		}
		
		if (d.gpioData.A0Mode != '0') {
			localStorage.setItem('a0Name', d.gpioData.A0Name);
			localStorage.setItem('a0Info', 'Monitor "' + d.gpioData.A0Name + '"');
		} else {
			localStorage.setItem('a0Name', 'Unused');
			localStorage.setItem('a0Info', 'A0 is unused');
		}
	}
	
	if(localStorage.deviceClassString)
		$("#deviceType").text(localStorage.deviceClassString);
	if (localStorage.firmwareVersion)
		$("#version").text(localStorage.firmwareVersion);
		
	for (var i = 0; i < 5; i++) {
		a = $('#d' + i);
		if(localStorage['d' + i + 'Name']) {
			a.text(localStorage['d' + i + 'Name']);
			localStorage['d' + i + 'Name'] != 'Unused' ? a.parent().show() : a.parent().hide();
		}
		if(localStorage['d' + i + 'Info']) {
			a.next().attr('data-content', localStorage['d' + i + 'Info']);
		}
	}
	
	a = $("#a0");
	if(localStorage.a0Name) {
		a.text(localStorage.a0Name);
		localStorage.a0Name != 'Unused' ? a.parent().show() : a.parent().hide(); 
	}
	if(localStorage.a0Info) {
		a.next().attr('data-content', localStorage.a0Info);
	}
	
	$('#bodySection').fadeIn();
	
	return update;
})();

$.ajaxSetup({
	cache : false
});
	
$(document).ready(function() {
	
	updateUI();
	
	$('[data-po=po]').popover();
	
	$.ajax({
		url : 'getHomePageData',
		dataType : 'json',
		timeout : 2000
	}).done(function(d, textStatus, jqXHR) {
		updateUI(d);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getHomePageData ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});
});

$('#confirmDlg').on('show.bs.modal', function(event) {
	var lnk = $(event.relatedTarget);
	var action = lnk.data('action');

	var modal = $(this);

	switch (action) {
		case 'reboot':
			modal.find('.modal-title').text('Device Reboot');
			modal.find('.modal-footer #actionBtn').text('Reboot');
			modal.find('.modal-footer #actionBtn').data('action', action);
			modal.find('.modal-body div').text('Ok to reboot now?');
			break;
		case 'reset':
			modal.find('.modal-title').text('Device Reset');
			modal.find('.modal-footer #actionBtn').text('Reset');
			modal.find('.modal-footer #actionBtn').data('action', action);
			modal.find('.modal-body div').html('Are you sure you want to reset this device to factory default settings?<br/>All existing configuration will be lost.');
			break;
		}
});

$('.modal-footer #actionBtn').on('click', function(event) {
	var btn = $(event.target);
	var action = btn.data('action');
	window.location = "/" + action;
});
