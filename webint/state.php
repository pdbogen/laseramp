<?php
	include_once( "db.php" );
	print( json_encode( Array(
		"daemon_state" => state( "daemon_state" ),
		"daemon_pid"   => state( "daemon_pid" ),
		"daemon_device_list" => json_decode( state( "daemon_device_list" ) ),
		"error" => NULL,
		"bounds" => Array(
			"min" => Array(
				"x" => state( "current_min_x" ),
				"y" => state( "current_min_y" )
			),
			"max" => Array(
				"x" => state( "current_max_x" ),
				"y" => state( "current_max_y" )
			)
		),
		"pulse" => time() - state( "daemon_ping" ),
		"current_num_lines" => state( "current_num_lines" ),
		"current_line" => state( "current_line", null, 0 ),
		"daemon_error" => state( "daemon_error", null, "" )
	) ) );
?>
