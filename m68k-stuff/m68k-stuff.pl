#!/usr/bin/perl

$beginmagic = 'begin:';    # const char magic[6];
$beginmagicformat = 'A6';
$endmagic = ':end';
$endmagicformat = 'A4';
$prefix = "/*\n * See m68k-stuff.s\n\ */\n\n#include \"m68k_stuff_dump.h\"\n\n";
$postfix = "\n";

$format = 'H2';

open(F,"m68k-stuff.o") || die("open failed");
$beginoffset = 0;
while( sysread( F, $var, length(pack($beginmagicformat, 0))) ) {
    ($name) = unpack( $beginmagicformat, $var );
    last if ( $name =~ /^$beginmagic/s );
    $beginoffset = ++$beginoffset;
    sysseek( F, $beginoffset, SEEK_SET ) || die "can't seek";
}

$beginoffset = $beginoffset + 8;
$endoffset = $beginoffset;
while( sysread( F, $var, length(pack($endmagicformat, 0))) ) {
    ($name) = unpack( $endmagicformat, $var );
    last if ( $name =~ /^$endmagic/s );
    $endoffset = ++$endoffset;
    sysseek( F, $endoffset, SEEK_SET ) || die "can't seek";
}

$offset = $beginoffset;
print "$prefix";
$dumpsize = $endoffset;
$dumpsize -= $beginoffset;

print "int m68k_stuff_dump_size = $dumpsize;\n";

print "B m68k_stuff_dump[] = {\n\t";
$cnt = 1;
sysseek( F, $offset, SEEK_SET ) || die "can't seek";
while( $offset != $endoffset ) {
    sysread( F, $var, length(pack($format,0)));
    ($value) = unpack( $format, $var );
    print "0x$value";
    $offset = ++$offset;
    print ", " if ( $offset != $endoffset );
    if ( $cnt == 8 ) {
	print "\n\t";
	$cnt = 0;
    }
    $cnt = ++$cnt;
}
print "\n};";
print "$postfix";

close(F);
