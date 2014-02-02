/**
 * dsf2flac - http://code.google.com/p/dsf2flac/
 * 
 * A file conversion tool for translating dsf dsd audio files into
 * flac pcm audio files.
 * 
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
 * Acknowledgements
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
  * fstream_plus.h
  * 
  * Header file for extended version of fstream.
  * 
  * Provides some more convinient methods for reading and checking at the same time
  * Also for reading data in reverse bit order
  * 
  */
  
#ifndef FILEPLUS_H
#define FILEPLUS_H

#include <fstream>
#include "dsf2flac_types.h"

typedef dsf2flac_uint64 stream_size;

class fstreamPlus : public std::fstream
{
public:
	fstreamPlus();
	virtual ~fstreamPlus();
	
	/** Overload seekg methods to return true on fail **/
	bool seekg(streampos pos);
	bool seekg(streamoff pos, ios_base::seekdir way);
	
	/** Additional read methods - native bit order **/
	// All return true on error. All read "n" numbers (not n chars/bytes!)
	bool read_int8 		(dsf2flac_int8*     b,	stream_size n);
	bool read_uint8		(dsf2flac_uint8*	b,	stream_size n);
	bool read_uint16	(dsf2flac_uint16*	b,	stream_size n);
	bool read_uint32	(dsf2flac_uint32*	b,	stream_size n);
	bool read_uint64	(dsf2flac_uint64*	b,	stream_size n);
	
	
	/** Additional read methods - reverse byte order **/
	// All return true on error. All read "n" numbers (not n chars/bytes!)
	bool read_int8_rev 	(dsf2flac_int8*     b,stream_size n) { return read_int8 (b,n); };
	bool read_int32_rev	(dsf2flac_int32*   b,stream_size n);
	bool read_uint8_rev	(dsf2flac_uint8*	b,stream_size n) { return read_uint8(b,n); };
	bool read_uint16_rev(dsf2flac_uint16*   b,stream_size n);
	bool read_uint32_rev(dsf2flac_uint32*   b,stream_size n);
	bool read_uint64_rev(dsf2flac_uint64*	b,stream_size n);
	
	
	/** Extra things **/
	template<typename rType> static void reverseByteOrder(rType* b,stream_size n);
	template<typename rType> static rType reverseByteOrder(rType b);
	
	char* getFilePath();
	
private:

	/** templates for the readers **/
	template<typename rType> bool read_helper(rType* b, stream_size n);
	template<typename rType> bool read_helper_rev(rType* b, stream_size n);
	

};

#endif // FILEPLUS_H
