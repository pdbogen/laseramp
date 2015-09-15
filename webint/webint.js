var device_list;

function refresh() {
	jQuery.get( "state.php", null, update, "json" );
	setTimeout( refresh, 1000 );
}

function sendCommand(cmd,val) {
	if( val ) {
		jQuery.post( "/", { "cmd": cmd, "val": val } );
	} else {
		jQuery.post( "/", { "cmd": cmd } );
	}
}

function update( data ) {
	daemon_state( data );
}

function daemon_state( data ) {
	var state = data[ "daemon_state" ];
	if( state == "stopped" ) {
		$("#dstate")
			.empty()
			.append( document.createTextNode( state + " [ " ) )
			.append(
				$("<a>")
					.attr( "href", "#" )
					.click( function() { sendCommand( "startdaemon" ); return false; } )
					.text( "start" )
			)
			.append( document.createTextNode( " ]" ) );
	} else {
		$("#dstate")
			.empty()
			.append( document.createTextNode( state + " [ " ) )
			.append(
				$("<a>")
					.attr( "href", "#" )
					.click( function() { sendCommand( "stopdaemon" ); return false; } )
					.text( "stop" )
			)
			.append( document.createTextNode( " ]" ) );
	}
	if( state == "waiting" ) {
		jsmessage = "";
		$(".printparam").prop( "disabled", false );
	} else {
		jsmessage = "Note: Daemon is not waiting for new SVG at this time. Start daemon or cancel current job to upload new graphics.";
		$(".printparam").prop( "disabled", true );
	}
	if( data[ "daemon_error" ] != "" ) {
		jsmessage += "<br/>" + data[ "daemon_error" ];
	}
	$("#lag").text( data[ "pulse" ] );
	update_message();
	update_device_list( data[ "daemon_device_list" ] );
	update_current_job( state, data[ "bounds" ], data[ "current_num_lines" ], data[ "current_line" ] );
}

function update_device_list( deviceArray ) {
//	if( device_list != 
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

function update_current_job( state, bounds, num_lines, cur_lines ) {
	if( state == "ready") {
		$("div.progressbox").slideUp();
		$("div.readybox").slideDown();
		$("#min_x").text( bounds["min"]["x"] );
		$("#min_y").text( bounds["min"]["y"] );
		$("#max_x").text( bounds["max"]["x"] );
		$("#max_y").text( bounds["max"]["y"] );
		$("#bounds_width").text( bounds["max"]["x"] - bounds["min"]["x"] );
		$("#bounds_height").text( bounds["max"]["y"] - bounds["min"]["y"] );
		$("#current_num_lines").text( num_lines );
	} else if( state == "begin" || state == "running" ) {
		$("div.readybox").slideUp();
		$("div.progressbox").slideDown();
		var bar = $("div.progressbox .progressbar");
		bar.progressbar({
			max: num_lines
		});
		bar.progressbar('value', cur_lines);
		bar.progressbar( 'enable' );
		$("div.progresslabel").html( cur_lines + "/" + num_lines + " " + Math.round( cur_lines / num_lines * 100 ) + "%" );
	} else {
		$("div.progressbox").slideUp();
		$("div.readybox").slideUp();
	}
}

$(document).ready( function() {
	$(".printparam").prop( "disabled", true );
	daemon_state( state );
	setTimeout( refresh, 1000 );
} );
