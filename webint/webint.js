function refresh() {
	jQuery.get( "state.php", null, update, "json" );
	setTimeout( refresh, 1000 );
}

function update( data ) {
	daemon_state( data );
}

function daemon_state( data ) {
	var state = data[ "daemon_state" ];
	if( state == "stopped" ) {
		$("#dstate").html( state + " [ <a href='?cmd=startdaemon'>start</a> ]" );
	} else {
		$("#dstate").html( state + " [ <a href='?cmd=stopdaemon'>stop</a> ]" );
	}
	if( state == "waiting" ) {
		jsmessage = "";
		$(".printparam").prop( "disabled", false );
	} else {
		jsmessage = "Note: Daemon is not waiting for new SVG at this time. Start daemon or cancel current job to upload new graphics.";
		$(".printparam").prop( "disabled", true );
	}
	update_message();
}

function update_message() {
	var msg="";
	if( phpmessage != "" ) {
		msg = phpmessage;
	}
	if( phpmessage != "" && jsmessage != "" ) {
		msg += "<br/>";
	}
	if( jsmessage != "" ) {
		msg += jsmessage;
	}
	$("#error").html( msg );
}

$(document).ready( function() {
	$(".printparam").prop( "disabled", true );
	daemon_state( state );
	setTimeout( refresh, 1000 );
} );
