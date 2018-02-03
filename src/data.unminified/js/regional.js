$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$('[data-po=po]').popover();

	$.ajax({
		url : 'getRegionalSettings',
		dataType : 'json',
		timeout : 2000
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = '/login.cm';
		} else {
			console.log('getRegionalSettings ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});
	
	function setNtpPnlState(ntpEnabled) {
		if(ntpEnabled) 
			$('#ntpPnl').fadeIn();
		else
			$('#ntpPnl').fadeOut();
	}
	
	function convertMonthStrToZeroBasedMonth(month) {
		var intmonth = parseInt(month);
		return intmonth - 1;
	}

	function initState(ntp) {

		ntp.ntpEnabled == "true" ? $("#ntpEnabled").prop('checked', true) : $("#ntpEnabled").prop('checked', false);
		setNtpPnlState(ntp.ntpEnabled == "true");
		
		$("#ntpServer").val(ntp.ntpServer);
		$("#ntpTimeZone").val(ntp.ntpTimeZone);
		$("#ntpSyncInterval").val(ntp.ntpSyncInterval);
		$("#ntpLocale").val(ntp.ntpLocale);
		
		if (ntp.ntpEnabled == "true") {
			
			if(ntp.syncResponseReceived == "true") {
				
				var month = convertMonthStrToZeroBasedMonth(ntp.month);
				var ntpDt = new Date(ntp.year, month, ntp.day, ntp.hour, ntp.minute, ntp.second);
				$('#tdNtpTime').text(ntpDt.toLocaleDateString(ntp.ntpLocale) + ' ' + ntpDt.toLocaleTimeString(ntp.ntpLocale));
				
				var lastInterval = new Date().getTime();
				window.setInterval(
					function() {
						
						var nowMsecs = new Date().getTime();
						var diffMsecs = nowMsecs - lastInterval;
						
						ntpDt.setMilliseconds(ntpDt.getMilliseconds() + diffMsecs);
						$('#tdNtpTime').text(ntpDt.toLocaleDateString(ntp.ntpLocale) + ' ' + ntpDt.toLocaleTimeString(ntp.ntpLocale));
						lastInterval = new Date().getTime();
						
					}, 1000);
				
				month = convertMonthStrToZeroBasedMonth(ntp.lastSyncMonth);
				var lastSyncDt = new Date(ntp.lastSyncYear, month, ntp.lastSyncDay, ntp.lastSyncHour, ntp.lastSyncMinute, ntp.lastSyncSecond);
				$('#tdLastSync').text(lastSyncDt.toLocaleDateString(ntp.ntpLocale) + ' ' + lastSyncDt.toLocaleTimeString(ntp.ntpLocale));
				$('#tdLastSyncRetries').text(ntp.lastSyncRetries + ' (of 5)');
				$('#ntpResultPnl').fadeIn();
				
			} else {
				
				$('#ntpResultPnl').removeClass('panel-success').addClass('panel-danger');
				$('#ntpResultPnl .panel-body').text('No NTP time sync response has been received. NTP time sync only works when the device is confgured as a WiFi client connected to the internet. Check device network configuration and your internet connection.');
				$('#ntpResultPnl').fadeIn();
				
			}
			
		}
		
		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};

	$(document).on('change', '#ntpEnabled', function() {
		setNtpPnlState($(this).prop('checked'));
		$('button[type=submit]').show();
	});
	$(document).on('change', '#ntpServer, #ntpTimeZone, #ntpSyncInterval, #ntplocale', function() {
		$('button[type=submit]').show();
	});
	
});
