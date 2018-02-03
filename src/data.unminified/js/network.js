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

	$('.ip_address').mask('0ZZ.0ZZ.0ZZ.0ZZ', {
		translation : {
			'Z' : {
				pattern : /[0-9]/,
				optional : true
			}
		},
		clearIfNotMatch : true
	});

	$.ajax({
		url : 'getNetSettings',
		dataType : 'json',
		timeout : 2000
	}).done(function(data, textStatus, jqXHR) {
		initState(data);
	}).fail(function(jqXHR, textStatus, errorThrown) {
		if (jqXHR.status == '403') {
			window.location = '/login.cm';
		} else {
			console.log('getNetSettings ajax call failure', jqXHR, textStatus, errorThrown);
		}
	});

	function setWifiModePnlState(wifiMode) {
		switch(wifiMode) {
		case '1':
			$('#apPnl').hide();
			$('#cliPnl').show();
			break;
		case '2':
			$('#apPnl').show();
			$('#cliPnl').hide();
			break;
		case '3':
			$('#apPnl').show();
			$('#cliPnl').show();
			break;
		}
	};

	function initState(ns) {

		$("#wifiMode").val(ns.wifiMode);
		setWifiModePnlState(ns.wifiMode);

		$('#netApIpMode').val(ns.netApIpMode);
		ns.netApIpMode == '0' ? $('#netApStaticIpPnl').hide() : $('#netApStaticIpPnl').show();

		$('#apSSID').val(ns.apSSID);
		$('#apPwd').val(ns.apPwd);
		$('#apChannel').val(ns.apChannel);

		$('#netApSSID').val(ns.netApSSID);
		$('#netApPwd').val(ns.netApPwd);
		
		var unassignedIP = '0.0.0.0';

		if (ns.netApStaticIp != unassignedIP)
			$('#netApStaticIp').val(ns.netApStaticIp);
		if (ns.netApSubnet != unassignedIP)
			$('#netApSubnet').val(ns.netApSubnet);
		if (ns.netApGatewayIp != unassignedIP)
			$('#netApGatewayIp').val(ns.netApGatewayIp);
		if (ns.netApDnsIp != unassignedIP)
			$('#netApDnsIp').val(ns.netApDnsIp);
		if (ns.netApDns2Ip != unassignedIP)
			$('#netApDns2Ip').val(ns.netApDns2Ip);

		$('div.alert.alert-success').hide();
		$('#mainPnl').fadeIn();
	};

	function checkValid() {
		var valid = true;

		function ipValid(elemId, valId, minLength) {
			var val = true;
			if (($(elemId).val().length == 0) || ((minLength) && $(elemId).val().length < minLength)) {
				val = false;
				$(valId).css('display', 'block');
			} else {
				$(valId).css('display', 'none');
			}
			return val;
		}

		var wm = $("#wifiMode").val();

		if (wm == '2' || wm == '3') {
			valid &= ipValid('#apSSID', '#apSSIDValFail');
			valid &= ipValid('#apPwd', '#apPwdValFail', 8);
		}
		if (wm == '1' || wm == '3') {
			valid &= ipValid('#netApSSID', '#netApSSIDValFail');
			valid &= ipValid('#netApPwd', '#netApPwdValFail', 8);

			if ($("#netApIpMode").val() == '1') {
				valid &= ipValid('#netApStaticIp', '#netApStaticIpValFail');
				valid &= ipValid('#netApSubnet', '#netApSubnetValFail');
				valid &= ipValid('#netApGatewayIp', '#netApGatewayIpValFail');
				valid &= ipValid('#netApDnsIp', '#netApDnsIpValFail');
				valid &= ipValid('#netApDns2Ip', '#netApDns2IpValFail');
			}
		}

		return valid;
	};

	$(document).on('change', '#wifiMode', function() {
		setWifiModePnlState($(this).val());
		$('button[type=submit]').show();
	});
	$(document).on('change', '#netApIpMode', function() {
		$(this).val() == '0' ? $('#netApStaticIpPnl').fadeOut() : $('#netApStaticIpPnl').fadeIn();
		$('button[type=submit]').show();
	});
	$(document).on('keydown', 'input', function() {
		$('button[type=submit]').show();
	});
	$(document).on('focus', '#apPwd, #netApPwd', function() {
		$(this)[0].type = 'text';
	});
	$(document).on('blur', '#apPwd, #netApPwd', function() {
		$(this)[0].type = 'password';
	});

	$(document).on('click', 'button[type=submit]', function(e) {
		if (!checkValid()) {
			e.preventDefault();
		}
	});
});
