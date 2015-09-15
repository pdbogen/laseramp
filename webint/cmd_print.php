<?php
function cmd_print() {
	// Uploaded SVG
	if( array_key_exists( "svgfile", $_FILES ) ) {
		$file = $_FILES[ "svgfile" ][ "tmp_name" ];
		$newname = tempnam("/tmp/", "svg");
		rename( $file, $newname );
		chmod( $newname, 0644 );
		state("current_file", $newname);
		state("current_file_name", $_FILES[ "svgfile" ][ "name" ] );
	} else {
		$_SESSION[ "message" ] = "Please select a file to engrave.";
	}
	foreach( Array("activepower", "feedrate", "inactivepower", "travelrate") as $i ) {
		if( array_key_exists( "savedefaults", $_POST ) ) {
			state("ui_default_$i", intval($_POST[$i]));
		}
		state("current_$i", intval($_POST[$i]));
	}

	if( array_key_exists( "savedefaults", $_POST ) ) {
		state("ui_default_dpmm", floatval($_POST["dpmm"]));
	}
	state("current_dpmm", floatval($_POST["dpmm"]));

	state( "daemon_state", "parse" );
	header( "Location: /" );
	exit;
}
?>
