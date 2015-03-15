<?php
	$mysql = new mysqli( "localhost", "root", "taiph1ohwei0ieHieGhae5alu" );
	$mysql->query( "CREATE DATABASE IF NOT EXISTS laseramp" );
	$mysql->select_db( "laseramp" );
	$mysql->query( "CREATE TABLE IF NOT EXISTS state ( `key` VARCHAR(128) UNIQUE PRIMARY KEY, value TEXT )" );
	function state( $key, $value=NULL, $default=NULL ) {
		global $mysql;
		if( $value === NULL ) {
			$query = $mysql->prepare( "SELECT value FROM state WHERE `key`=?" );
			if( $query === FALSE ) {
				print( "Error occurred during prepare retrieving state $key from database: ".$mysql->error );
				return NULL;
			}
			$query->bind_param( "s", $key );
			if( !$query->execute() ) {
				print( "Error occurred during execute retrieving state $key from database: ".$mysql->error );
				return NULL;
			}
			$query->bind_result( $value );
			if( $query->fetch() === FALSE ) {
				print( "Error occurred during fetch retrieving state $key from database: ".$mysql->error );
				return NULL;
			}
			$query->close();
			if( $value === NULL ) {
				return $default;
			} else {
				return $value;
			}
		} else {
			$query = $mysql->prepare( "INSERT INTO state (`key`,value) VALUES (?,?) ON DUPLICATE KEY UPDATE value=?" );
			if( $query === FALSE ) {
				print( "Error occurred during prepare setting state $key to database: ".$mysql->error );
				return NULL;
			}
			$query->bind_param( "sss", $key, $value, $value );
			if( !$query->execute() ) {
				print( "Error occurred during execute setting state $key to database: ".$mysql->error );
				return NULL;
			}
			return TRUE;
		}
	}
?>
