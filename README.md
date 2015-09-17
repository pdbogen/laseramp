# laseramp
GCode-based firmware for cheap Chinese laser engravers

# webint

A PHP web interface for easier "slicing" and burning. Just give it an SVG, it'll
hand it off to svg2gcode, let you sanity-check the results, then transmit the
gcode to the engraver.

I run this on a Raspberry Pi connected to the arduino that drives the printer
itself. Wireless laser engraving! The future.

# daemon.pl

This is the 'worker' for the web interface. It can be automatically started by
the web interface.

# gccon.pl

A simple GCode console with local buffering. Think of it like a super bare bones
Pronterface.

# svg2gcode.pl

Parse and convert SVG to GCode for the printer. Currently, curves aren't
supported; patches implementing biarc interpolation are welcome. Only *paths*
are converted; make sure to convert any native objects to paths before
attempting to GCodify.

Note: I've found it most effective to simply treat whatever my native unit in my
SVG program as millimeters, and leave --dpmm at the default of 1. E.g., my
Inkscape drawings are "53 pixels" by "53 pixels". By leaving --dpmm at 1, this
equates to an engraving 53mm x 53mm.

	svg2gcode.pl --file <file.svg> --dpmm <dots per MM>
	             --feedrate <movement while "active" lasing>
	             --travelrate <movement while "inactive">
	             --active <laser power while active lasing, max 255>
	             --inactive <laser power while inactive, max 255>
	             --debug --json

Note about inactive laser power: My laser takes a few fractions of a second to
begin lasing properly if --inactive is too low, which would cause my lines to
miss a few millimeters at the beginning. You should set --inactive as high as
you can so that it doesn't mark the material, which will minimize any start-up
time.

# Firmware
Arduino firmware for driving RAMPS. Expects X and Y stepper motors and
mechanical endstops controlled through the appropriately labeled pins.
Currently, Y endstop is a "min" endstop and X endstop is a "max" endstop.

Requires a special laser driver controlled via D9. I control power directly, but
you could easily control TTL instead. Something like:
http://www.aixiz.com/store/product_info.php/cPath/28/products_id/104

Happy to run a fan connected to D10. I use the fan to cool the laser itself
while operating.

The firmware automatically disables the laser and motors any time there's no
lasing command executing.
