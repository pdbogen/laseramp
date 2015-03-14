#!/usr/bin/env perl

use warnings;
use strict;

use DBI;
use Getopt::Long;

my $db_host = "localhost";
my $db_user = "root";
my $db_pass = "taiph1ohwei0ieHieGhae5alu";
my $db_db   = "laseramp";
my $dbh = undef;

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

sub main {
	my $daemon = undef;
	GetOptions(
		"daemon|d" => \$daemon,
	);

	if( !defined( $daemon ) || fork() == 0 ) {
		if( !defined( $daemon ) || fork() == 0 ) {
			state( "daemon_pid", $$ );
			$SIG{INT}  = sub { state( "daemon_state", "stopped" ); exit; };
			$SIG{TERM} = $SIG{INT};
			$dbh = DBI->connect( "DBI:mysql:host=$db_host", $db_user, $db_pass );
			$dbh->do( "CREATE DATABASE IF NOT EXISTS $db_db" );
			$dbh->do( "USE $db_db" );
			state( "daemon_state", "running" );
			while( state( "daemon_state" ) ne "stopping" ) {
				sleep 1;
			}
			state( "daemon_state", "stopped" );
		}
	} else {
		wait();
	}
}

main();
