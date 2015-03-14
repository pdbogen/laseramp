<?php
	include_once( "db.php" );
	print( json_encode( Array(
		"daemon_state" => state( "daemon_state" ),
		"daemon_pid"   => state( "daemon_pid" ),
		"error" => NULL
	) ) );
?>
