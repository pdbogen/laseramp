<?php
	include_once( "db.php" );
	$daemon_state = state( "daemon_state" );
	if( $_GET[ "cmd" ] == "stopdaemon" ) {
		state( "daemon_state", "stopping" );
		header( "Location: /" );
		exit;
	}
	if( $_GET[ "cmd" ] == "startdaemon" ) {
		if( $daemon_state == "stopped" ) {
			exec( "../daemon.pl >/dev/null 2>/dev/null </dev/null &" );
		}
		header( "Location: /" );
		exit;
	}
?>
<html>
<head>
<script src='https://code.jquery.com/jquery-2.1.3.min.js' type='text/javascript'></script>
<script src='webint.js' type='text/javascript'></script>
</head>
<body>
Daemon State: <span id='dstate'></span>
</body>
</html>
