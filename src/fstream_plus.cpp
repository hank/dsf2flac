/*
 * dsf2flac - http://code.google.com/p/dsf2flac/
 * 
 * A file conversion tool for translating dsf dsd audio files into
 * flac pcm audio files.
 *
 * Copyright (c) 2013 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 
 * Acknowledgments
 * 
 * Many thanks to the following authors and projects whose work has greatly
 * helped the development of this tool.
 * 
 * 
 * Sebastian Gesemann - dsd2pcm (http://code.google.com/p/dsd2pcm/)
 * SACD Ripper (http://code.google.com/p/sacd-ripper/)
 * Maxim V.Anisiutkin - foo_input_sacd (http://sourceforge.net/projects/sacddecoder/files/)
 * Vladislav Goncharov - foo_input_sacd_hq (http://vladgsound.wordpress.com)
 * Jesus R - www.sonore.us
 * 
 */
 
 /**
  * fstream_plus.cpp
  * 
  * Implementation file for extended version of fstream.
  * 
  * Provides some more convinient methods for reading and checking at the same time
  * Also for reading data in reverse bit order
  * 
  */

#include "fstream_plus.h"

fstreamPlus::fstreamPlus() : std::fstream()
{
}

fstreamPlus::~fstreamPlus()
{
}

/** Overload seekg methods to return true on fail **/
bool fstreamPlus::seekg(streampos pos)
{
	std::fstream::seekg(pos);
	return !good();
}
bool fstreamPlus::seekg(streamoff pos, ios_base::seekdir way)
{
	std::fstream::seekg(pos,way);
	return !good();
}

/** Additional read methods - native bit order **/
bool fstreamPlus::read_int8   (dsf2flac_int8*   b,stream_size n) {return read_helper(b,n);}
bool fstreamPlus::read_uint8  (dsf2flac_uint8*  b,stream_size n) {return read_helper(b,n);}
bool fstreamPlus::read_uint16 (dsf2flac_uint16* b,stream_size n) {return read_helper(b,n);}
bool fstreamPlus::read_uint32 (dsf2flac_uint32* b,stream_size n) {return read_helper(b,n);}
bool fstreamPlus::read_uint64 (dsf2flac_uint64* b,stream_size n) {return read_helper(b,n);}

/** Additional read methods - reverse byte order **/
bool fstreamPlus::read_int32_rev	(dsf2flac_int32*  b,stream_size n) {return read_helper_rev(b,n);}
bool fstreamPlus::read_uint16_rev	(dsf2flac_uint16* b,stream_size n) {return read_helper_rev(b,n);}
bool fstreamPlus::read_uint32_rev	(dsf2flac_uint32* b,stream_size n) {return read_helper_rev(b,n);}
bool fstreamPlus::read_uint64_rev	(dsf2flac_uint64* b,stream_size n) {return read_helper_rev(b,n);}

/** templates for the readers **/
template<typename rType> bool fstreamPlus::read_helper(rType* b, stream_size n) {
	read( reinterpret_cast<char*>(b), sizeof(rType)*n);
	return bad();
}
template<typename rType> bool fstreamPlus::read_helper_rev(rType* b, stream_size n) {
	read( reinterpret_cast<char*>(b), sizeof(rType)*n);
	reverseByteOrder(b,n);
	return bad();
}


/** Extra things **/
template<typename rType> void fstreamPlus::reverseByteOrder(rType* b,stream_size n)
{
	for (stream_size i=0; i<n; i++)
		b[i] = reverseByteOrder(b[i]);
}
template<typename rType> rType fstreamPlus::reverseByteOrder(rType b)
{
	int n = sizeof(rType);
	rType b2;
	unsigned char* dst = reinterpret_cast<unsigned char*>(&b2);
	unsigned char* src = reinterpret_cast<unsigned char*>(&b);
	for (int i=0;i<n;i++)
		dst[i] = src[n-1-i];
	return b2;
}
