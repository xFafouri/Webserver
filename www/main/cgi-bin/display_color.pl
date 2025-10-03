#!/usr/bin/perl

use strict;
use warnings;

# Default color
my $color = "white";

# Get color from POST or GET
if ($ENV{'REQUEST_METHOD'} eq 'POST' || $ENV{'REQUEST_METHOD'} eq 'GET') {
    my $input = '';
    if ($ENV{'REQUEST_METHOD'} eq 'POST') {
        read(STDIN, $input, $ENV{'CONTENT_LENGTH'});
    } else {
        $input = $ENV{'QUERY_STRING'} // '';
    }

    if ($input =~ /color=([^&]+)/) {
        $color = $1;
    }
}

# Decode URL encoding
$color =~ tr/+/ /;
$color =~ s/%([0-9A-Fa-f]{2})/chr(hex($1))/eg;

# Print HTTP header
print "Content-Type: text/html\n\n";

# Print HTML with full-page background color
print <<"HTML";
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Color Page</title>
    <style>
        body {
            margin: 0;
            height: 100vh;
            background-color: $color;
        }
    </style>
</head>
<body>
</body>
</html>
HTML
