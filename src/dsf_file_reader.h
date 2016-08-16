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

#ifndef DSFFILEREADER_H
#define DSFFILEREADER_H

#include <dsd_sample_reader.h> // Base class: dsdSampleReader
#include <fstream_plus.h>

/**
 * This class extends dsdSampleReader providing access to dsd samples and other info
 * from dsf files.
 *
 * Some of the rarer features of dsf are not well tested due to a lack of files:
 * 8bit dsd
 */
class DsfFileReader : public DsdSampleReader
{
public:
	/** Class constructor.
	 *  filePath must be a valid dsf file location.
	 *  If there is an issue reading or loading the file then isValid() will be false.
	 */
	DsfFileReader(char* filePath);
	/** Class destructor.
	 *  Closes the file and frees the internal buffers.
	 */
	virtual ~DsfFileReader();
public: // methods overriding dsdSampleReader

	dsf2flac_uint32 getSamplingFreq() {return samplingFreq;};
	bool step();
	void rewind();
	dsf2flac_int64 getLength() {return sampleCount;};
	dsf2flac_uint32 getNumChannels() {return chanNum;};
	bool msbIsPlayedFirst() { return true;}
	bool samplesAvailable() { return !file.eof() && DsdSampleReader::samplesAvailable(); }; // false when no more samples left
	ID3_Tag getID3Tag(dsf2flac_uint32 trackNum) {return metadata;}
public:
	/// Can be called to display some useful info to stdout.
	void dispFileInfo();
private:
	/// Allocates the block buffer which holds the dsd data read from the file for when it is required by the circular buffer.
	void allocateBlockBuffer();
	/// Reads lots of info from the file.
	bool readHeaders();
	/// Attempts to read the metadata from the end of the dsf file.
	void readMetadata();
	/// This private function is called whenever new data from the file is needed for the block buffer.
	bool readNextBlock();
	/// A handy little helper for checking idents.
	static bool checkIdent(dsf2flac_int8* a, dsf2flac_int8* b); // MUST be used with the char[4]s or you'll get segfaults!
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
};

#endif // DSFFILEREADER_H
