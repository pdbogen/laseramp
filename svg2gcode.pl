#!/usr/bin/env perl

use warnings;
use strict;

use XML::Bare;
use Getopt::Long;
use Data::Dumper;
use JSON;

my $dpmm = undef;
my @bounds = ( [undef,undef], [undef,undef] );
my @cur_pos = (0,0);
my @subpath_start = (0,0);
my $debuglevel = 0;

my $feedrate = 100;
my $travelrate = 1000;
my $active = 255;
my $inactive = 1;

sub main {
	my $file = undef;
	my $dpi = undef;
	my $help = undef;
	my @paths;

	GetOptions(
		"file|f=s"         => \$file,
		"dpmm|D=f"         => \$dpmm,
		"dpi|d=f"          => \$dpi,
		"feedrate=i"       => \$feedrate,
		"travelrate=i"     => \$travelrate,
		"active=i"         => \$active,
		"inactive=i"       => \$inactive,
		"debug|verbose|v+" => \$debuglevel,
		"help"             => \$help,
	);

	usage() if defined $help;

	if( defined( $dpi ) && defined( $dpmm ) ) {
		warn( "--dpi and --dpmm are mutually-exclusive" );
		usage();
	}

	if( defined( $dpi ) ) {
		$dpmm = $dpi / 25.4;
	} elsif( !defined( $dpmm ) ) {
		$dpmm = 1;
	}

	debug( 1, "Pixel to mm conversion factor: Pixels / $dpmm" );

	unless( defined $file ) {
		warn( "--file is required" );
		usage();
	}

	my $svg = svg_from_file( $file );
	die( "cannot find SVG element in XML file" ) unless exists $svg->{ "svg" };
	$svg = $svg->{ "svg" };

	extract_lines( \@paths, $svg );
	calculate_bounds( \@paths, \@bounds );
	flip_y_axis( $svg, \@paths );
	calculate_bounds( \@paths, \@bounds );
	adjust_origin( \@paths, \@bounds );
	calculate_bounds( \@paths, \@bounds );
	print( STDERR "Drawing bounds: " );
	print( STDERR "(".$bounds[0]->[0].",".$bounds[1]->[0].") -> " );
	print( STDERR "(".$bounds[0]->[1].",".$bounds[1]->[1].")\n" );
	#fill_paths( $svg, \@paths );
	print( lines_to_gcode( \@paths ) );
}

sub adjust_origin {
	my( $paths, $bounds ) = @_;
	for my $path (@$paths) {
		my $lines = $path->{ "lines" };
		for( my $i = 0; $i < scalar @$lines; $i++ ) {
			if( $bounds[1]->[0] < 0 ) {
				$lines->[ $i ]->{ "srcY" } -= $bounds[1]->[0];
				$lines->[ $i ]->{ "y" }    -= $bounds[1]->[0];
			}
			if( $bounds[0]->[0] < 0 ) {
				$lines->[ $i ]->{ "srcX" } -= $bounds[0]->[0];
				$lines->[ $i ]->{ "x" }    -= $bounds[0]->[0];
			}
		}
	}
}

sub fill_paths {
	my $svgPath = shift;
	my $paths = shift;
	my $nPaths = scalar @$paths;
	for( my $i = 0; $i < $nPaths; $i++ ) {
		if( $paths->[$i]->{ "fill-rule" } eq "nonzero" ) {
			fill_paths_nonzero( $paths, $paths->[$i] );
		}
	}
}

sub fill_paths_nonzero {
	my $paths = shift;
	my $path = shift;
	my @bounds = (0,0);
	my @fill_lines;
	my @candidates;
	# Find bounds
	# From y=Ymin to Ymax
	# Find lines where srcY <= y and maxY >= y
	# 	For l in Lines
	#		l->srcY == l->dstY -> skip
	#       l->srcY > y && l->dstY < y -> use, negative
	#       l->srcY < y && l->dstY > y -> use, positive
	# n.b.: y-y1=m*(x-x1)
	for my $line (@{$path->{ "lines" }}) {
		$line->{ "srcY" } == $line->{ "y" } and next;
		# Ignore travel moves. FIXME: We should explicitly flag travel moves.
		$line->{ "e" } == $inactive and next;

		if( $line->{ "srcY" } > $line->{ "y" } ) {
			$line->{ "direction" } = "negative";
		} else {
			$line->{ "direction" } = "positive";
		}
		if( $line->{ "srcX" } == $line->{ "x" } ) {
			$line->{ "slope" } = undef;
		} else {
			$line->{ "slope" } = ($line->{ "srcY" } - $line->{ "y" }) / ($line->{ "srcX" } - $line->{ "x" });
		}
		$bounds[0] = min( $bounds[0], min( $line->{ "srcY" }, $line->{ "y" } ) );
		$bounds[1] = max( $bounds[1], max( $line->{ "srcY" }, $line->{ "y" } ) );
		push @candidates, $line;
	}

	my $lastend = undef;
	for( my $y = $bounds[0]; $y <= $bounds[1]; $y+=.2 ) {
		my @icepts;
		# Find intersecting lines, calculate intersect X with pos/neg
		for my $line (@candidates) {
			if( $line->{ "srcY" } < $y && $line->{ "y" } > $y ||
			    $line->{ "srcY" } > $y && $line->{ "y" } < $y ) {
			    if( !defined( $line->{ "slope" } ) ) {
			    	push @icepts, [ $line->{ "srcX" }, $line->{ "direction" } ];
			    } else {
					push @icepts, [ ($y-$line->{ "srcY" }) / $line->{ "slope" } + $line->{ "srcX" }, $line->{ "direction" } ];
				}
			}
		}
		@icepts = sort { $a->[0] <=> $b->[0] } @icepts;
		my( $start, $end, $count );
		$count = 0;
		for my $i (@icepts) {
			if( $i->[1] eq "positive" ) {
				if( $count == 0 ) {
					$start = $i->[0];
				}
				$count++;
			} else {
				$count--;
				if( $count == 0 ) {
					$end = $i->[0];
					#print( " Last: $lastend\n" );
					#print( "Start: $start\n" );
					#print( "  End: $end\n" );
					if( !defined( $lastend ) || abs( $lastend - $start ) < abs( $lastend - $end ) ) {
						push @fill_lines, { "x" => $start, "y" => $y, "e" => $inactive, "f" => $travelrate };
						push @fill_lines, { "x" => $end, "y" => $y, "e" => $active, "f" => $feedrate };
						$lastend = $end;
					} else {
						push @fill_lines, { "x" => $end, "y" => $y, "e" => $inactive, "f" => $travelrate };
						push @fill_lines, { "x" => $start, "y" => $y, "e" => $active, "f" => $feedrate };
						$lastend = $start;
					}
				}
			}
		}
	}
	push @$paths, { "lines" => \@fill_lines };
}

sub min {
	return ($_[0]<$_[1]?$_[0]:$_[1]);
}

sub max {
	return ($_[0]>$_[1]?$_[0]:$_[1]);
}

sub lines_to_gcode {
	my $paths = shift;
	my $r = "";
	for my $path (@$paths) {
		for my $line (@{$path->{ "lines" }}) {
			$r .= "G1 X".$line->{ "x" }." Y".$line->{ "y" }." E".$line->{ "e" }." F".$line->{ "f" }."\n";
		}
	}
	return $r;
}

# SVG standard is that the Y0 is the top, X0 is the left. That's OK for X, but Y
# is backwards.

# The lowest Y give the gap between the top of the drawing and the y-axis.
# The height less the highest Y value gives the gap between the bottom of the drawing and the _new_ Y-axis.

sub flip_y_axis {
	my( $svg, $paths ) = @_;
	for my $path (@$paths) {
		my $lines = $path->{ "lines" };
		for( my $i = 0; $i < scalar @$lines; $i++ ) {
			if( $svg->{ "height" }->{ "value" } =~ m/^([0-9.]+)/ ) {
				my $h = $1;
				$lines->[ $i ]->{ "y" } = $h / $dpmm - $lines->[ $i ]->{ "y" };
				$lines->[ $i ]->{ "srcY" } = $h / $dpmm - $lines->[ $i ]->{ "srcY" };
			}
		}
	}
}

sub calculate_bounds {
	my $paths = shift;
	my $bounds = shift;
	$bounds->[0]->[0] = undef;
	$bounds->[0]->[1] = undef;
	$bounds->[1]->[0] = undef;
	$bounds->[1]->[1] = undef;

	for my $path (@$paths) {
		for my $line (@{$path->{ "lines" }}) {
			if( !defined $bounds->[0]->[0] || $line->{ "x" } < $bounds->[0]->[0] ) {
				$bounds->[0]->[0] = $line->{ "x" };
			}

			if( !defined $bounds->[0]->[1] || $line->{ "x" } > $bounds->[0]->[1] ) {
				$bounds->[0]->[1] = $line->{ "x" };
			}

			if( !defined $bounds->[1]->[0] || $line->{ "y" } < $bounds->[1]->[0] ) {
				$bounds->[1]->[0] = $line->{ "y" };
			}

			if( !defined $bounds->[1]->[1] || $line->{ "y" } > $bounds->[1]->[1] ) {
				$bounds->[1]->[1] = $line->{ "y" };
			}
		}
	}
}

# Parse through groups and root paths
sub extract_lines {
	my $paths = shift;
	my $svg = shift;

	extract_lines_from_group( $paths, $svg );
#	# Groups can contains paths and/or groups.
#	if( exists( $svg->{ "g" } ) ) {
#		extract_lines_from_group( $paths, $svg->{ "g" } );
#	}
#	if( exists( $svg->{ "path" } ) ) {
#		my $path = { "lines" => [] };
#		extract_lines_from_group( $path, { "transform" => [], "path" => $svg->{ "path" } }, 0 );
#		push @$paths, $path;
#	}
#	die( "no paths or groups found in SVG" ) unless (exists $svg->{ "path" } || exists $svg->{ "g" });

}

sub extract_lines_from_group {
	my $paths = shift;
	my $group = shift;
	my $transforms = (shift or []);
	my $bc = (shift or []);

	# If there's just one group, it's a non-array singleton; if there are more, 
	# it's an arrayref. convert singletons to arrayrefs.
	$group = [ $group ] unless ref( $group ) eq "ARRAY";

	my $group_no = 0;
	for my $el (@$group) {
		unshift @$transforms, $el->{ "transform" } if exists $el->{ "transform" };
		push @$bc, "group".($group_no++);
		if( exists( $el->{ "transform" } ) ) {
			debug(3, join( "->", @$bc )." xf = ".encode_json($el->{ "transform" }) );
		} else {
			debug(3, join( "->", @$bc )." xf = nil" );
		}
#		extract_lines_from_group( $path, $svg->{ "g" }->[ $i ], $i );
		if( exists( $el->{ "path" } ) ) {
			debug( 3, "Parsing paths from ".join( "->", @$bc ) );
			my $p = $el->{ "path" };
			$p = [ $p ] unless ref( $p ) eq "ARRAY";
			my $path = { "lines" => [] };
			for( my $i = 0; $i < scalar @$p; $i++ ) {
				unshift @$transforms, $p->[ $i ]->{ "transform" } if exists $p->[ $i ]->{ "transform" };
				push @$bc, "path$i";
				debug( 3, "parsing ".join( "->", @$bc )." w/ xf: ".encode_json( $transforms ) );
				unless( extract_lines_from_path( $path, $p->[ $i ], $i, $transforms ) ) {
					warn( "error occured parsing ".join( "->", @$bc ) );
				}
				pop @$bc;
				shift @$transforms if exists $p->[ $i ]->{ "transform" };
			}
			push @$paths, $path
		}
		if( exists( $el->{ "g" } ) ) {
			debug( 3, "Recursing groups in ".join( "->", @$bc ) );
			extract_lines_from_group( $paths, $el->{ "g" }, $transforms, $bc );
		}
		pop @$bc;
		shift @$transforms if exists $el->{ "transform" };
	}

#	my $t = $group->{ "transform" }; $t = [ $t ] unless ref( $t ) eq "ARRAY";

#	warn( "no paths found in group #".($index+1) ) unless exists $group->{ "path" };
#	my $p = $group->{ "path" };
#	$p = [ $p ] unless ref( $p ) eq "ARRAY";

#	for( my $i = 0; $i < scalar @$p; $i++ ) {
#		unless( extract_lines_from_path( $lines, $p->[ $i ], $i, $t ) ) {
#			warn( "error occured parsing path #".$index." in group #".$index );
#		}
#	}
#	push @$paths, $path;
}

sub calculate_fill {
	my( $path, $svgPath ) = @_;
	# For now, ignore fill color and just fill according to fill-opacity
	$path->{ "fill" } = $svgPath->{ "fill-opacity" };
	$path->{ "fill-rule" } = $svgPath->{ "fill-rule" } // "nonzero";
}

sub extract_lines_from_path {
	my $path    = shift;
	my $svgPath = shift;
	my $index   = shift;
	my $transforms = shift;

	return 1 if $svgPath->{ "d" }->{ "value" } eq "1";

	my $lines = $path->{ "lines" };

	calculate_fill( $path, $svgPath );

	# Parse out style, convert fill color / fill-opacity to zig-zag density
	# maybe do something with lins

	# Analyze path data; currently just straight segments, biarc comes later

	my $mode = undef;
	my @data = split( / /, $svgPath->{ "d" }->{ "value" } );
	# Reset current position
	@cur_pos       = (undef,undef);
	@subpath_start = (0,0);
	while( my $cmd = shift @data ) {
		if( $cmd eq "m" ) {
			unless( parse_moveto( $lines, $index, \@data, $transforms, "relative" ) ) {
				warn( "failed to parse relative moveto in path #$index" );
				return 0;
			}
		} elsif( $cmd eq "M" ) {
			unless( parse_moveto( $lines, $index, \@data, $transforms, "absolute" ) ) {
				warn( "failed to parse relative moveto in path #$index" );
				return 0;
			}
		} elsif( $cmd eq "L" ) {
			unless( parse_lineto( $lines, $index, \@data, $transforms, "absolute" ) ) {
				warn( "failed to parse absolute lineto in path #$index" );
				return 0;
			}
		} elsif( $cmd eq "l" ) {
			unless( parse_lineto( $lines, $index, \@data, $transforms, "relative" ) ) {
				warn( "failed to parse relative lineto in path #$index" );
				return 0;
			}
		} elsif( $cmd eq "z" or $cmd eq "Z" ) {
			my $line = transform_line( { "srcX" => $cur_pos[0], "srcY" => $cur_pos[1], "x" => $subpath_start[0], "y" => $subpath_start[1], "e" => $active, "f" => $feedrate }, $transforms );
			push @$lines, $line;
			debug( 2, sprintf( "(%d,%d)z>(%d,%d)\n", $line->{ "srcX" }, $line->{ "srcY" }, $line->{ "x" }, $line->{ "y" } ) );
#		} elsif( $cmd eq "c" ) {
#			unless( parse_curveto( $lines, $index, \@data, $groupTransforms, "relative" ) ) {
#				warn( "failed to parse Bézier curve in path #$index" );
#				return 0;
#			}
		} else {
			warn( "unhandled command $cmd in data for path #$index: ".encode_json( $svgPath ) );
			return 0;
		}
	}
	return 1;
}

sub transform_line {
	my $line = shift;
	my $transforms = shift;
	my $s = [ $line->{ "srcX" }, $line->{ "srcY" } ];
	apply_transforms( $s, $transforms );
	my $d = [ $line->{ "x" }, $line->{ "y" } ];
	apply_transforms( $d, $transforms );
	debug( 3, sprintf( "Transformed line (%d,%d)->(%d,%d) to (%d,%d)->(%d,%d)", $line->{ "srcX" }, $line->{ "srcY" }, $line->{ "x" }, $line->{ "y" }, $s->[0], $s->[1], $d->[0], $d->[1] ) );
	$line->{ "srcX" } = $s->[0];
	$line->{ "srcY" } = $s->[1];
	$line->{ "x" } = $d->[0];
	$line->{ "y" } = $d->[1];
	return $line;
}

sub parse_moveto {
	my( $lines, $index, $data, $groupTransforms, $absolute ) = @_;
	# Parse out the first relative coordinate, update cur_pos and subpath_start.
	# delegate to parse_lineto in relative mode
	my $new_pos;
	if( !defined $cur_pos[0] ) {
		# The first coordinate of a moveto is absolute if it's the first moveto
		# of a path, even if the moveto is relative.
		$new_pos = parse_coordinate( $data, "absolute" );
	} else {
		$new_pos = parse_coordinate( $data, $absolute );
	}

	if( !defined $new_pos ) {
		warn( "moveto command with no or invalid starting coordinate in path#$index" );
		return 0;
	}

	my $line;
	if( !defined $cur_pos[0] ) {
		$line = transform_line( { "srcX" => 0, "srcY" => 0, "x" => $new_pos->[0], "y" => $new_pos->[1], "e" => $inactive, "f" => $travelrate }, $groupTransforms );
		$line->{ "srcX" } = 0;
		$line->{ "srcY" } = 0;
	} else {
		$line = transform_line( { "srcX" => $cur_pos[0], "srcY" => $cur_pos[1], "x" => $new_pos->[0], "y" => $new_pos->[1], "e" => $inactive, "f" => $travelrate }, $groupTransforms );
	}
	push @$lines, $line;
	debug( 2, sprintf( "(%d,%d)m>(%d,%d)\n", $line->{ "srcX" }, $line->{ "srcY" }, $line->{ "x" }, $line->{ "y" } ) );
	@cur_pos = @$new_pos;
	@subpath_start = @cur_pos; # moveto either starts a path or a subpath, either way, update subpath start

	parse_lineto( $lines, $index, $data, $groupTransforms, $absolute );
}

sub parse_lineto {
	# For each transformed coordinate read off the stack,
	# apply it as relative appropriate

	my( $lines, $index, $data, $groupTransforms, $absolute ) = @_;

	while(1) {
		my $new_pos = parse_coordinate( $data, $absolute );

		# This isn't necessarily an error.
		return 1 unless defined $new_pos;

		# lineto updates current position but not subpath start

		my $line = transform_line( { "srcX" => $cur_pos[0], "srcY" => $cur_pos[1], "x" => $new_pos->[0], "y" => $new_pos->[1], "e" => $active, "f" => $feedrate }, $groupTransforms );
		push @$lines, $line;
		debug( 2, sprintf( "(%d,%d)=>(%d,%d)\n", $line->{ "srcX" }, $line->{ "srcY" }, $line->{ "x" }, $line->{ "y" } ) );
		@cur_pos = @$new_pos;
	};
}

sub parse_coordinate {
	my( $data, $absolute ) = @_;
	if( $absolute eq "relative" ) {
		return parse_relative_coordinate( $data );
	} else {
		return parse_absolute_coordinate( $data );
	}
}

sub parse_relative_coordinate {
	# Parse out a transformed absolute coordinate and add it to cur_pos
	my $data = shift;

	my $c = parse_absolute_coordinate( $data );
	return undef unless defined $c;

	$c->[0] += $cur_pos[0];
	$c->[1] += $cur_pos[1];
	debug( 3, "Absolutified coordinate: (".$c->[0].", ".$c->[1].")" );

	return $c;
}

sub parse_absolute_coordinate {
	# If next item on stack is a valid coordinate, parse it out; otherwise, fail up
	# Apply transformations
	# Return
	my $data = shift;

	unless( defined( $data->[0] ) && $data->[0] =~ /^-?[0-9.]+(e-?\d+)?,-?[0-9.]+(e-?\d+)?$/) {
		return undef;
	}

	my $c = [ split( /,/, shift @$data ) ];
	debug( 3, "Read raw coordinate: (".$c->[0].", ".$c->[1].")" );
	return $c;
}

sub apply_transforms {
	my( $coords, $groupTF ) = @_;
	for my $tf ( @$groupTF ) {
		next unless defined $tf;
		die( "invalid transform $tf, expected hashref" ) unless ref( $tf ) eq "HASH";
		apply_transform( $coords, $tf->{ "value" } );
		debug( 3, "Applied transform ".$tf->{ "value" }." -> (".$coords->[0].",".$coords->[1].")" );
	}
	debug( 3, "Applied ".(scalar @$groupTF)." transforms" );

	$coords->[0] /= $dpmm;
	$coords->[1] /= $dpmm;
}

sub apply_transform {
	my( $coords, $tf ) = @_;
	return unless $tf;
	if( $tf =~ m/^translate\((-?[0-9.]+(?:e-?[0-9.]+)?),(-?[0-9.]+(?:e-?[0-9.]+)?)\)$/i ) {
		$tf = "matrix(1,0,0,1,$1,$2)";
	}
	if( $tf =~ m/^matrix\((-?[0-9.]+(?:e-?[0-9.]+)?),(-?[0-9.]+(?:e-?[0-9.]+)?),(-?[0-9.]+(?:e-?[0-9.]+)?),(-?[0-9.]+(?:e-?[0-9.]+)?),(-?[0-9.]+(?:e-?[0-9.]+)?),(-?[0-9.]+(?:e-?[0-9.]+)?)\)$/i ) {
		debug( 3, "Applying Matrix [ $1 $3 $5 | $2 $4 $6 | 0 0 1 ]" );
		my $x = $1 * $coords->[0] + $3 * $coords->[1] + $5;
		my $y = $2 * $coords->[0] + $4 * $coords->[1] + $6;
		$coords->[0] = $x;
		$coords->[1] = $y;
		return 1;
	} else {
		warn( "unrecognized transform $tf" );
		return 0;
	}
}

sub svg_from_file {
	my $f = shift;
	die( "$f does not appear to be readable" ) unless( -r $f );
	my $parser = new XML::Bare( file => $f );
	return $parser->parse();
}

sub usage {
	print( STDERR "usage: $0 --file <filename.svg> [--dpmm <#> or --dpi <#>]\n" );
	print( STDERR "	--file, -f    SVG file to parse and convert to GCode\n" );
	print( STDERR "	--dpi, -d     Provide DPI value to convert pixels to millimeters\n" );
	print( STDERR "	--dpmm, -D    Provide dots per mm value to convert pixels to millimeters\n" );
	print( STDERR "	--debug, -v   Provide debugging output\n" );
	print( STDERR "	--feedrate    Feed rate for burn/engrave moves in mm/min (default: 100 )\n" );
	print( STDERR "	--travelrate  Feed rate for travel moves in mm/min (default: 1000)\n" );
	print( STDERR " --active      Power level for laser when executing a cutting move (default: 255)\n" );
	print( STDERR " --inactive    Power level for laser when executing a travel move (default: 1)\n" );
	print( STDERR "	  --verbose\n" );
	exit(1);
}

sub debug {
	my( $level, $message ) = @_;
	if( $level <= $debuglevel ) {
		print( STDERR $message, "\n" );
	}
}


main();
