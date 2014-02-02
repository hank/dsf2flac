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
  * dsf_file_reader.h
  * 
  * Header file for the class dsfFileReader.
  * 
  * This class extends dsdSampleReader providing acces to dsd samples and other info
  * from dsf files.
  * 
  * Some of the rarer features of dsf are not well tested due to a lack of files:
  * dsd64
  * 8bit dsd
  * 
  */

#ifndef DSFFILEREADER_H
#define DSFFILEREADER_H

#include "dsd_sample_reader.h" // Base class: dsdSampleReader
#include "fstream_plus.h"

class dsfFileReader : public dsdSampleReader
{
public:
	// constructor and destructor
	dsfFileReader(char* filePath);
	virtual ~dsfFileReader();
public:
	// methods overriding dsdSampleReader
	bool step();
	void rewind();
	dsf2flac_int64 getLength() {return sampleCount;};
	dsf2flac_uint32 getNumChannels() {return chanNum;};
	dsf2flac_uint32 getSamplingFreq() {return samplingFreq;};
	bool samplesAvailable() { return !file.eof() && dsdSampleReader::samplesAvailable(); }; // false when no more samples left
	ID3_Tag getID3Tag(dsf2flac_uint32 trackNum) {return metadata;}
public:
	// other public methods
	void dispFileInfo();
private:
	// private variables
	char* filePath;
	fstreamPlus file;
	// below store file info
	dsf2flac_uint64 fileSz;
	dsf2flac_uint64 metaChunkPointer;
	dsf2flac_uint64 sampleDataPointer;
	dsf2flac_uint64 dataChunkSz;
	dsf2flac_uint32 formatVer;
	dsf2flac_uint32 formatID;
	dsf2flac_uint32 chanType;
	dsf2flac_uint32 chanNum;
	dsf2flac_uint32 samplingFreq;
	dsf2flac_uint64 sampleCount; //per channel
	dsf2flac_uint32 blockSzPerChan;
	ID3_Tag metadata;
	// vars to hold the data and mark position
	dsf2flac_uint8** blockBuffer; // used to store blocks of raw data from the file
	dsf2flac_int64 blockCounter; // stores the index to the current blockBuffer
	dsf2flac_int64 blockMarker; // stores the current position in the blockBuffer
	// private methods
	void allocateBlockBuffer();
	bool readHeaders();
	void readMetadata();
	bool readNextBlock();
	static bool checkIdent(dsf2flac_int8* a, dsf2flac_int8* b); // MUST be used with the char[4]s or you'll get segfaults!
};

#endif // DSFFILEREADER_H
