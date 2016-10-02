$('[data-po=po]').popover();

$(document).on('click', '#loginFrm button[type=submit]', function(e){
	if($('#pwdIp').val().length < 8) {
		$('#pwd').addClass('has-error');
		$('#pwdValFail').show();
		e.preventDefault();
	}
});