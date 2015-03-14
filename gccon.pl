#!/usr/bin/env perl

use warnings;
use strict;

use Device::SerialPort;
use Getopt::Long;
use IO::Select;

my $ready_to_run = 0;
my $serial_ready = 0;
my $user_ready = 0;
my $serial_buffer = "";
my $user_buffer  = "";
my $eof = 0;
my $running = 1;
my $timeout = 5;
my $lastio = -1;

sub main {
	my $port = undef;
	my $baud = 9600;
	my $file = undef;
	my $gcFH = *STDIN;

	GetOptions(
		"port|p=s" => \$port,
		"baud|b=i" => \$baud,
		"file|f=s" => \$file,
		"timeout|t=i" => \$timeout,
	);

	unless( defined( $port ) ) { warn( "missing --port specifying serial device /dev node" ); usage(); exit; }

	if( defined( $file ) ) {
		open $gcFH,"<",$file or
			die( "couldn't open $file for reading: $!" );
	}

	my $oPort = new Device::SerialPort( $port, undef, undef ) or
		die( "failed to open serial port $port: $!" );

	$oPort->baudrate( $baud ) or
		die( "failed to set baud rate to $baud: $!" );

	my $bytes;
	do {
		( undef, $bytes, undef, undef ) = $oPort->status;
		for( my $i = 0; $i < $bytes; $i++ ) {
			$oPort->read( 1 );
		}
	} while( $bytes > 0 );

	$oPort->pulse_dtr_on( 1000 );

	while( $running ) {
		loop( $gcFH, $oPort );
		serial_in( $oPort );
	}

	$oPort->close or
		warn( "failed to close serial port $port: $!" );
}

sub usage {
	print( STDERR "usage: $0 --port <port>\n" );
	print( STDERR "	--port, -p  specify device node representing serial port (required)\n" );
	print( STDERR "	--baud, -b  specify baud rate (default: 9600)\n" );
	print( STDERR "	--file, -f	read GCode from file instead of standard in (optional)\n" );
}

sub loop {
	my $userFH = shift;
	my $port = shift;
	if( $ready_to_run && $user_ready ) {
		READER:
		while( !$eof && bytes_available( $userFH ) ) {
			$lastio = time;
			my $c;
			my $n = sysread( $userFH, $c, 1 );
			$eof = ($n == 0);
			if( $c eq "\n" ) {
				print( "<- $user_buffer\n" );
				if( $user_buffer =~ /^\(.*/ ) {
					# Ignore
				} else {
					$user_ready = 0;
					$port->write( $user_buffer."\n" );
				}
				$user_buffer = "";
				last READER;
			} else {
				$user_buffer .= $c;
			}
		}
	}
	if( $serial_ready ) {
		$lastio = time;
		print( "\r-> '$serial_buffer'\n" );
		if( $serial_buffer =~ /^start$/i ) {
			$ready_to_run = 1;
			$port->write( "G1 X0 Y0 F1000\n" );
		}
		if( $serial_buffer =~ /^ok( |$)/i || 
		    $serial_buffer =~ /^!!/ ) {
			$user_ready = 1;
		}
		$serial_buffer = "";
		$serial_ready = 0;
	}
	if( $user_ready && !$eof ) {
		$|++; print( "\r\$ " ); $|--;
	}
	if( $eof && time > ($lastio+$timeout) ) {
		$running = 0;
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

main();
