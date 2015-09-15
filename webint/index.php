<?php
	session_start();
	include_once( 'db.php' );
	include_once( 'cmd_print.php' );

	$daemon_state = state( 'daemon_state' );

	if( $_POST['cmd'] == 'begin_print' ) {
		state( 'daemon_state', 'begin' );
		exit;
	}

	if( $_POST[ 'cmd' ] == 'stopdaemon' ) {
		state( 'daemon_state', 'stopping' );
		exit;
	}
	if( $_POST[ 'cmd' ] == 'startdaemon' ) {
		if( $daemon_state == 'stopped' ) {
			state( 'daemon_state', 'starting' );
			exec( '../daemon.pl >/tmp/daemon.log 2>&1 </dev/null &' );
		}
		exit;
	}

	if( $_POST['cmd'] == 'cancel' ) {
		state( 'daemon_state', 'cancel' );
		header( 'Location: /' );
		exit;
	}

	if( $_POST[ 'cmd' ] == 'print' ) {
		cmd_print();
	}

	if( $_POST['cmd'] == 'set_serial_port') {
		state( 'daemon_device', $_POST[ 'serial' ] );
		header( 'Location: /' );
		exit;
	}

	if( $_POST['cmd'] == 'setbaud' ) {
		state( 'daemon_baud', intval( $_POST['val'] ) );
		print( json_encode( Array( "error" => "" ) ) );
		exit;
	}
?>
<!DOCTYPE html>
<html>
<head>
	<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>
	<link rel='stylesheet' href='//code.jquery.com/ui/1.11.4/themes/smoothness/jquery-ui.css'>
	<script src='https://code.jquery.com/jquery-2.1.3.min.js' type='text/javascript'></script>
	<script src='https://code.jquery.com/ui/1.11.4/jquery-ui.min.js' type='text/javascript'></script>
	<script src='webint.js' type='text/javascript'></script>
	<script type='text/javascript'>
		var phpmessage='<?=htmlentities( $_SESSION[ 'message' ] );?>';
		var state=<?php include_once( 'state.php' ); ?>;
	</script>
	<link rel='stylesheet' type='text/css' href='webint.css'>
</head>
<body>
<table>
	<tr><td>&nbsp;</td><td><span id='error' class='error'></span></td></tr>
</table>

<div class='pane' id='daemonpane'>
	<div class='divtitle'>Daemon Settings</div>
	<table>
		<tr><td>Daemon State:</td><td><span id='dstate'></span> (<span id='lag'></span>s)</td></tr>
		<tr><td>Serial Device:</td>
			<td>
				<form method='post'>
				<input type='hidden' name='cmd' value='set_serial_port'>
				<select name='serial' style='font-size: 65%;'>
					<option></option>
<?
	$dev = state('daemon_device');
	foreach(json_decode(state('daemon_device_list')) as $port) {
		print( '<option' );
		if( $port == $dev ) {
			print( ' selected' );
		}
		print( '>' . htmlentities( $port ) . '</option>\n' );
	}
?>
				</select> <input type='submit' value='Set'>
				</form>
			</td></tr>
		<tr>
			<td>Baud Rate:</td>
			<td>
				<select name='baud' id='baud'>
<?
	$dbaud = state( 'daemon_baud' );
	foreach(Array(300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200) as $baud) {
		print( '<option' );
		if( $dbaud == $baud ) {
			print( ' selected' );
		}
		print( '>' . $baud . '</option>' );
	}
?>
				</select>
				<input type='button' value='Set' onclick='sendCommand("setbaud", $("#baud").val()); return false;'>
			</td>
		</tr>
	</table>
	<div class='readybox' style='display: none;'>
		<div class='divtitle'>Ready to etch!</div>
		<span>4. Check Dimensions:</span>
		<table style='margin-left: 1em;'>
			<tr><td>Minimum</td><td>(<span id='min_x'></span>,<span id='min_y'></span>)</td></tr>
			<tr><td>Maximum</td><td>(<span id='max_x'></span>,<span id='max_y'></span>)</td></tr>
			<tr><td>Width</td><td><span id='bounds_width'></span></td></tr>
			<tr><td>Height</td><td><span id='bounds_height'></span></td></tr>
			<tr><td>Segments</td><td><span id='current_num_lines'></span></td></tr>
		</table>
		<div style='text-align: center;'>
			[ <a onclick='sendCommand("cancel"); return false;' href='#'>cancel</a> ]
			[ <a onclick='sendCommand("begin_print"); return false;' href='#'>print</a> ]
		</div>
	</div>
	<div class='progressbox' style='display: none;'>
		<div class='divtitle'>Etching in progress!</div>
		<div class='progressbar'>
			<div class='progresslabel'></div>
		</div>
		<div style='text-align: center;'>
			[ <a onclick='sendCommand("cancel"); return false;' href='#'>cancel</a> ]
		</div>
	</div>
</div>

<div class='pane'>
	<div class='divtitle'>SVG Conversion</div>
	<form action='/' method='POST' id='printform' enctype='multipart/form-data'>
	<table>
		<tr>
			<td>1. Select SVG File to print:</td>
			<td>
					<input type='hidden' name='MAX_FILE_SIZE' value='2000000' class='printparam' />
					<input type='hidden' name='cmd' value='print' class='printparam' />
					<input type='file' name='svgfile' class='printparam' />
			</td>
		</tr>
		<tr>
			<td>2. Change conversion parameters:</td>
			<td>
				<table>
					<tr><td>Active Laser Power:</td><td><input type='input' name='activepower' value='<?=state( 'ui_default_activepower', null, 255 )?>' class='printparam' size=4></td></tr>
					<tr><td>Active Laser Feedrate:</td><td><input type='input' name='feedrate' value='<?=state( 'ui_default_feedrate', null, 100 )?>' class='printparam' size=4></td></tr>
					<tr><td>Inactive Laser Power:</td><td><input type='input' name='inactivepower' value='<?=state( 'ui_default_inactivepower', null, 1 )?>' class='printparam' size=4></td></tr>
					<tr><td>Inactive Laser Travel rate:</td><td><input type='input' name='travelrate' value='<?=state( 'ui_default_travelrate', null, 1000 )?>' class='printparam' size=4></td></tr>
					<tr><td>Scaling factor (native units/px to mm):</td><td><input type='input' name='dpmm' value='<?=state( 'ui_default_dpmm', null, 1 )?>' class='printparam' size=4></td></tr>
					<tr><td>Save Defaults?</td><td><input type='checkbox' name='savedefaults' class='printparam'></td></tr>
				</table>
			</td>
		<tr><td>3. Convert to GCode:</td><td><input type='button' value='Convert' onclick='document.getElementById( "printform" ).submit();' class='printparam'></td></tr>
	</table>
	</form>
</div>
</body>
</html>
<?php
	unset( $_SESSION[ 'message' ] );
?>
