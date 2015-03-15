<?php
	session_start();
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
	if( $_POST[ "cmd" ] == "print" ) {
		// Uploaded SVG
		if( array_key_exists( "svgfile", $_FILES ) ) {
			$file = $_FILES[ "svgfile" ][ "tmp_name" ];
//			proc_open( "../svg2gcode.-pl -f
		} else {
			$_SESSION[ "message" ] = "Please select a file to engrave.";
		}
		// Parse SVG, check error state
		// If no errors, dump gcode into database
		// Set daemon state to "start".
		header( "Location: /" );
		exit;
	}
?>
<!DOCTYPE html>
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
	<script src='https://code.jquery.com/jquery-2.1.3.min.js' type='text/javascript'></script>
	<script src='webint.js' type='text/javascript'></script>
	<script type='text/javascript'>
		var phpmessage="<?=htmlentities( $_SESSION[ "message" ] );?>";
		var state=<?php include_once( "state.php" ); ?>;
	</script>
	<link rel='stylesheet' type='text/css' href='webint.css'>
</head>
<body>
<table>
	<tr><td>&nbsp;</td><td><span id='error' class='error'></span></td></tr>
	<tr><td>Daemon State:</td><td><span id='dstate'></span></td></tr>
	<tr>
		<td>1. Select SVG File to print:</td>
		<td>
			<form action='/' method='POST' id='printform' enctype='multipart/form-data'>
				<input type="hidden" name="MAX_FILE_SIZE" value="2000000" class='printparam' />
				<input type='hidden' name='cmd' value='print' class='printparam' />
				<input type='file' name='svgfile' class='printparam' />
			</form>
		</td>
	</tr>
	<tr>
		<td>2. Change conversion parameters:</td>
		<td>
			<table>
				<tr><td>Active Laser Power:</td><td><input type='input' name='activepower' value='<?=state( "ui_default_activepower", null, 255 )?>' class='printparam' size=4></td></tr>
				<tr><td>Active Laser Feedrate:</td><td><input type='input' name='feedrate' value='<?=state( "ui_default_feedrate", null, 100 )?>' class='printparam' size=4></td></tr>
				<tr><td>Inactive Laser Power:</td><td><input type='input' name='inactivepower' value='<?=state( "ui_default_inactivepower", null, 1 )?>' class='printparam' size=4></td></tr>
				<tr><td>Inactive Laser Travel rate:</td><td><input type='input' name='travelrate' value='<?=state( "ui_default_travelrate", null, 1000 )?>' class='printparam' size=4></td></tr>
				<tr><td>Scaling factor (native units/px to mm):</td><td><input type='input' name='dpmm' value='<?=state( "ui_default_dpmm", null, 1 )?>' class='printparam' size=4></td></tr>
				<tr><td>Save Defaults?</td><td><input type='checkbox' name='savedefaults' class='printparam'></td></tr>
			</table>
		</td>
	<tr><td>3. Convert to GCode:</td><td><input type='button' value='Convert' onclick='document.getElementById( "printform" ).submit();' class='printparam'></td></tr>
</table>
</body>
</html>
<?php
	unset( $_SESSION[ "message" ] );
?>
