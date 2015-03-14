function refresh() {
	jQuery.get( "state.php", null, update, "json" );
	setTimeout( refresh, 1000 );
}

function update( data ) {
	daemon_state( data );
}

function daemon_state( data ) {
	if( data[ "daemon_state" ] == "stopped" ) {
		$("#dstate").html( data[ "daemon_state" ] + " [ <a href='?cmd=startdaemon'>start</a> ]" );
	} else {
		$("#dstate").html( data[ "daemon_state" ] + " [ <a href='?cmd=stopdaemon'>stop</a> ]" );
	}
}

$(document).ready( function() {
	setTimeout( refresh, 1000 );
} );
