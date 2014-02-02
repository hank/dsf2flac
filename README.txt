dsf2flac - http://code.google.com/p/dsf2flac/

INTRODUCTION

A file conversion tool for translating dsf or dff dsd audio files into flac pcm
audio files.  I originally wrote this because I could not quite find any tool
which did exactly what I wanted.  I program quite a lot in java and I'm pretty
good at matlab. Although I've very occasionally written things in C and CPP,
this is really my first attempt at a full CPP program. I'm sure there are
errors, hopefully none to horrible.  I've tried to write things in a pretty
versatile/clear way. It should be possible to expand the functionality to add
other file types quite easily.  If you want to try different convesion filters
then it should be dead simple: just look in filters.cpp.

COMPILING

I developed and tested this program on Linux x86_64 with GCC. It probably
compile and works on x86 GCC too. Windows might be a bit more work.  If you are
building statically you'll need to link against these libs:

FLAC++
FLAC
boost_system
boost_timer
boost_filesystem
boost_chrono
id3
z
rt
ogg

The list is a little smaller if you want to use dynamic linking:

FLAC++
boost_system
boost_timer
boost_filesystem
id3

RUNNING

If you run "dsf2flac -h" you'll get a list of the options.

At the very simplest level you just need to run
"dsf2flac -i some_audio_file.dsf"


BENCHMARK

I was quite pleased with the performance.
For example, with the default filters (which are quite long) the program
will convert and encode from DSD64 to 24bit 88200Hz flac at around 
14x realtime on my ageing laptop (Intel Core2Duo P8600@2.40GHz)



LICENSE

Copyright (c) 2013 by respective authors.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


ACKNOWLEDGEMENTS

Many thanks to the following authors and projects whose work has greatly
helped the development of this tool.


Sebastian Gesemann - dsd2pcm (http://code.google.com/p/dsd2pcm/)
SACD Ripper (http://code.google.com/p/sacd-ripper/)
Maxim V.Anisiutkin - foo_input_sacd (http://sourceforge.net/projects/sacddecoder/files/)
Vladislav Goncharov - foo_input_sacd_hq (http://vladgsound.wordpress.com)
Jesus R - www.sonore.us
Erik Gregg - slight mods and github posting



