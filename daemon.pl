#!/usr/bin/env perl

use warnings;
use strict;

use Cwd 'abs_path';
use DBI;
use Getopt::Long;
use File::Find;
use JSON;
use Device::SerialPort;
use IO::Select;

my $db_host = "localhost";
my $db_user = "root";
my $db_pass = "taiph1ohwei0ieHieGhae5alu";
my $db_db   = "laseramp";
my $dbh = undef;

my $serial_ready = 0;
my $serial_buffer = "";
my $serial_port = undef;
my $serial_lastio = time;
my $serial_initialized = 0;
my $serial_idle = 0;

my @gcode;
my $gcode_index;

sub state {
	return undef unless defined $dbh;
	my $key = shift;
	my $value = shift;
	if( defined( $value ) ) {
		return $dbh->do( "INSERT INTO state (`key`,value) VALUES (?,?) ON DUPLICATE KEY UPDATE value=?", undef, ( $key, $value, $value ) );
	} else {
		my $statement = $dbh->prepare( "SELECT value FROM state WHERE `key`=?" );
		$statement->execute( $key );
		my @row = $statement->fetchrow_array();
		return $row[0];
	}
}

sub clear_state {
	return undef unless defined $dbh;
	my $key = shift;
	return $dbh->do( "DELETE FROM state WHERE `key`=?", undef, ( $key ) );
}

sub main {
	my $daemon = undef;
	GetOptions(
		"daemon|d" => \$daemon,
	);

	if( !defined( $daemon ) || fork() == 0 ) {
		if( !defined( $daemon ) || fork() == 0 ) {
			$SIG{INT}  = sub { state( "daemon_state", "stopped" ); exit; };
			$SIG{TERM} = $SIG{INT};
			$dbh = DBI->connect( "DBI:mysql:host=$db_host", $db_user, $db_pass );
			$dbh->do( "CREATE DATABASE IF NOT EXISTS $db_db" );
			$dbh->do( "USE $db_db" );
			state( "daemon_pid", $$ );
			state( "daemon_error", "" );
			state( "daemon_state", "waiting" );
			my $last_ping = time-1;
			my $state;
			while( state( "daemon_state" ) ne "stopping" ) {
				if( time > $last_ping ) {
					$state = state( "daemon_state" );
					state( "daemon_ping", time );
					$last_ping = time;
					update_serial_list();
				}
				loop($state);
				if( $state ne "running" ) {
					sleep 1;
				}
			}
			state( "daemon_state", "stopped" );
		}
	} else {
		wait();
	}
}

sub loop {
	my $state = shift;
	if( $state eq "parse" ) {
		parse_svg_file();
	} elsif( $state eq "cancel" ) {
		clean_sliced_file();
		shutdown_serial();
	} elsif( $state eq "begin" ) {
		state( "daemon_state", "running" );
		@gcode = @{ decode_json( state( "current_gcode" ) ) };
		$gcode_index = 0;
		setup_serial();
		$state = "running";
	}

	if( $state eq "running" ) {
		if( $gcode_index >= scalar @gcode ) {
			state( "daemon_state", "finishing" );
		} else {
			run_print();
		}
	}

	if( $state eq "finishing" ) {
		sleep(10);
		shutdown_serial();
		state( "daemon_state", "waiting" );
	}
}

sub shutdown_serial {
	if( defined $serial_port ) {
		$serial_port->close;
		undef $serial_port;
	}
}

sub setup_serial {
	my $port = state( "daemon_device" );
	my $baud = state( "daemon_baud", undef, 9600 );
	$serial_port = new Device::SerialPort( $port, undef, undef );

	if( !defined $serial_port ) {
		state( "daemon_error", "failed to open serial port $port: $!" );
		state( "daemon_state", "waiting" );
		return;
	}

	if( !$serial_port->baudrate( $baud ) ) {
		state( "daemon_error", "failed to set baud rate to $baud: $!" );
		state( "daemon_state", "waiting" );
		return;
	}

	$serial_port->pulse_dtr_on( 1000 );
}

sub run_print {
	serial_in( $serial_port );
	if( $serial_initialized && $serial_idle ) {
		$serial_idle = 0;
		$serial_port->write( $gcode[ $gcode_index ]."\n" );
		$gcode_index++;
		state( "current_line", $gcode_index );
	}

	if( $serial_ready ) {
		$serial_lastio = time;
		print( "-> '$serial_buffer'\n" );
		if( $serial_buffer =~ /^start$/i ) {
			$serial_initialized = 1;
			$serial_port->write( "G1 X0 Y0 F1000\n" );
		}
		if( $serial_buffer =~ /^ok( |$)/i ||
		    $serial_buffer =~ /^!!/ ) {
			$serial_idle = 1;
		}
		$serial_buffer = "";
		$serial_ready = 0;
	}
}

sub bytes_available {
	my $sel = IO::Select->new();
	my $fh = shift;
	my $handleName = shift;
	$sel->add( $fh );
	return $sel->can_read( 0 );
}

sub serial_in {
	return if( $serial_ready );
	my $port = shift;
	my $buf = undef;
	my $bytes;
	( undef, $bytes, undef, undef ) = $port->status;
	return if $bytes == 0;
	for( my $i = 0; $i < $bytes; $i++ ) {
		( undef, $buf ) = $port->read( 1 );
		next if $buf eq "\r";
		if( $buf eq "\n" ) {
			$serial_ready = 1;
			return;
		} else {
			$serial_buffer .= $buf;
		}
	}
}


sub update_serial_list {
	my @devs;
	find( sub { push(@devs, "/dev/serial/by-id/" . $_) unless $_ eq "." }, ( '/dev/serial/by-id/' ) );
	state( "daemon_device_list", encode_json \@devs );
}

sub parse_svg_file {
	my $file = state( "current_file" );
	my $dpmm = state( "current_dpmm" );
	my $feedrate = state( "current_feedrate" );
	my $travelrate = state( "current_travelrate" );
	my $activepower = state( "current_activepower" );
	my $inactivepower = state( "current_inactivepower" );
	my $cmdline;
	if( -x "svg2gcode.pl" ) {
		$cmdline = "./svg2gcode.pl -f $file --dpmm $dpmm --feedrate $feedrate --travelrate $travelrate --active $activepower --inactive $inactivepower --json";
	} elsif( -x "../svg2gcode.pl" ) {
		$cmdline = "../svg2gcode.pl -f $file --dpmm $dpmm --feedrate $feedrate --travelrate $travelrate --active $activepower --inactive $inactivepower --json";
	}
	print( "[I] Executing $cmdline...\n" );
	my $result = `$cmdline`;
	my $oRes = decode_json( $result );
	if( defined($oRes->{ "error" }) ) {
		state( "daemon_error", $oRes->{ "error" } );
		state( "daemon_state", "waiting" );
	} else {
		state( "current_min_x", $oRes->{ "bounds" }->{ "min" }->{ "x" } );
		state( "current_min_y", $oRes->{ "bounds" }->{ "min" }->{ "y" } );
		state( "current_max_x", $oRes->{ "bounds" }->{ "max" }->{ "x" } );
		state( "current_max_y", $oRes->{ "bounds" }->{ "max" }->{ "y" } );
		state( "current_gcode", encode_json( $oRes->{ "gcode" } ) );
		state( "current_num_lines", scalar @{ $oRes->{ "gcode" } } );
		state( "daemon_state", "ready" );
	}
	clear_state( "current_file" );       clear_state( "current_dpmm" );
	clear_state( "current_feedrate");    clear_state( "current_travelrate" );
	clear_state( "current_activepower"); clear_state( "current_inactivepower" );
	unlink( $file );
}

sub clean_sliced_file {
	clear_state( "current_min_x" ); clear_state( "current_min_y" );
	clear_state( "current_max_x" ); clear_state( "current_max_y" );
	clear_state( "current_gcode" );
	state( "daemon_state", "waiting" );
}

main();
