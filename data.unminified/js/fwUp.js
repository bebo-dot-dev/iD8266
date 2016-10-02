$(document).on('change', '.btn-file :file', function() {
	var input = $(this),
	    numFiles = input.get(0).files ? input.get(0).files.length : 1;
	input.trigger('fileselect', [numFiles]);
});

$(document).on('focus', 'body', function(){
	$('[data-tt=tt]').tooltip('hide');
});

$.fn.hasExtension = function(exts) {
    return (new RegExp('(' + exts.join('|').replace(/\./g, '\\.') + ')$')).test($(this).val());
};

$('.btn-file :file').on('fileselect', function(event, numFiles) {
	if (numFiles > 0) {
		if ($('#upFile').hasExtension(['.bin'])) {
			$("#fwValFail").removeClass("dib");
			$('.btn-file').hide();
			$('#msg').text("Uploading and flashing new firmware - do not navigate away from this page");
			window.setInterval(
				function(){
					$('#progress').text($('#progress').text() + '. ');
				}, 100);
			$(this).closest('form').submit();
		} else {
			$("#fwValFail").addClass("dib");
		}
	} else {
		$('[data-tt=tt]').tooltip('hide');
	}
});

$(function() {
	$('[data-tt=tt]').tooltip({
		'delay' : {
			'show' : 100,
			'hide' : 200
		}
	});
});
