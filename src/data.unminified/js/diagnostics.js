$.ajaxSetup({
	cache : false
});

$(document).ready(function() {

	$.ajax({
		url : 'getDiagnosticsData',
		dataType : 'json'
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = "/login.cm";
		} else {
			console.log('getDiagnosticsData ajax call failure', jqXHR, textStatus, errorThrown);
		}		
	});
	
	function convertMonthStrToZeroBasedMonth(month) {
		var intmonth = parseInt(month);
		return intmonth - 1;
	}
	
	function getWifiModeStr(wifiMode) {
		switch(wifiMode){
			case '1': return 'WiFi Client';
			case '2': return 'Access Point';
			case '3': return 'AP + WiFi Client';
			default: return 'Unknown';
		}
	}
	
	function getIpModeStr(netApIpMode) {
		switch(netApIpMode){
			case '0': return 'DHCP';
			case '1': return 'Static IP';
			default: return 'Unknown';
		}
	}

	function initState(diags) {

		var d = diags.runtimeData;
		$("#deviceClassString").text(d.deviceClassString);
		$("#firmwareVersion").text(d.firmwareVersion);
		$("#sdkVersion").text(d.sdkVersion);
		$("#coreVersion").text(d.coreVersion.replace(/_/g, '.'));
		$("#deviceMac").text(d.deviceMac);
		$("#deviceApIp").text(d.deviceApIp);
		$("#deviceIp").text(d.deviceIp);
		$("#clientIp").text(d.clientIp);
		$("#binaryBytes").text(d.binaryBytes);
		$("#freeBytes").text(d.freeBytes);
		$("#freeHeap").text(d.freeHeap);
		$("#deviceUpTime").text(d.deviceUpTime);
		
		d = diags.netSettings;
		$("#wifiMode").text(getWifiModeStr(d.wifiMode));
		$("#ipMode").text(getIpModeStr(d.netApIpMode));
		
		d = diags.dnsSettings;
		$("#hostName").text(d.hostName);
		$("#mdnsEnabled").text(d.mdnsEnabled);
		$("#dnsEnabled").text(d.dnsEnabled);
		
		d = diags.webserverSettings;
		$("#webserverPort").text(d.webserverPort);
		$("#websocketServerPort").text(d.websocketServerPort);
		
		d = diags.mqttData;
		$("#mqttSystemEnabled").text(d.mqttSystemEnabled);
		$("#mqttServerBrokerIp").text(d.mqttServerBrokerIp);
		$("#mqttServerBrokerPort").text(d.mqttServerBrokerPort);
		$("#mqttConnected").text(d.mqttConnected);
							
		d = diags.regionalSettings;
		$("#ntpEnabled").text(d.ntpEnabled);
		$("#ntpLocale").text(d.ntpLocale);
		
		if (d.ntpEnabled == "true") {
			
			if(d.syncResponseReceived == "true") {
				
				var month = convertMonthStrToZeroBasedMonth(d.month);
				var ntpDt = new Date(d.year, month, d.day, d.hour, d.minute, d.second);
				$('#ntpTime').text(ntpDt.toLocaleDateString(d.ntpLocale) + ' ' + ntpDt.toLocaleTimeString(d.ntpLocale));
				
				var lastInterval = new Date().getTime();
				window.setInterval(
					function() {
						
						var nowMsecs = new Date().getTime();
						var diffMsecs = nowMsecs - lastInterval;
						
						ntpDt.setMilliseconds(ntpDt.getMilliseconds() + diffMsecs);
						$('#ntpTime').text(ntpDt.toLocaleDateString(d.ntpLocale) + ' ' + ntpDt.toLocaleTimeString(d.ntpLocale));
						lastInterval = new Date().getTime();
						
					}, 1000);
				
				month = convertMonthStrToZeroBasedMonth(d.lastSyncMonth);
				var lastSyncDt = new Date(d.lastSyncYear, month, d.lastSyncDay, d.lastSyncHour, d.lastSyncMinute, d.lastSyncSecond);
				$('#ntpLastSync').text(lastSyncDt.toLocaleDateString(d.ntpLocale) + ' ' + lastSyncDt.toLocaleTimeString(d.ntpLocale));
				$('#ntpLastSyncRetries').text(d.lastSyncRetries + ' (of 5)');
				
			} else {
				
				$('#ntpLastSync').text('No NTP time sync response has been received. NTP time sync only works when the device is confgured as a WiFi client connected to the internet. Check device network configuration and your internet connection.');
				
			}
		}
		
		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};
});
