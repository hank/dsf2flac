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

#ifndef DSDIFFFILEREADER_H
#define DSDIFFFILEREADER_H

#include "dsd_sample_reader.h" // Base class: dsdSampleReader
#include "fstream_plus.h"
#include <boost/ptr_container/ptr_vector.hpp>


// this struct holds comments
typedef struct{
	dsf2flac_uint16		timeStampYear;
	dsf2flac_uint8		timeStampMonth;
	dsf2flac_uint8		timeStampDay;
	dsf2flac_uint8		timeStampHour;
	dsf2flac_uint8		timeStampMinutes;
	dsf2flac_uint16		cmtType;
	dsf2flac_uint16		cmtRef;
	dsf2flac_uint32		count;
	dsf2flac_int8*		commentText;
} DsdiffComment;

// this struct holds absolute start time
typedef struct{
	dsf2flac_uint16		hours;
	dsf2flac_uint8		minutes;
	dsf2flac_uint8		seconds;
	dsf2flac_uint32		samples;
} DsdiffAst;

// this struct holds markers
typedef struct{
	dsf2flac_uint16		hours;
	dsf2flac_uint8		minutes;
	dsf2flac_uint8		seconds;
	dsf2flac_uint32		samples;
	dsf2flac_int32		offset;
	dsf2flac_uint16		markType;
	dsf2flac_uint16		markChannel;
	dsf2flac_uint16		trackFlags;
	dsf2flac_uint32		count;
	dsf2flac_int8*		markerText;
} DsdiffMarker;

// this struct holds DST frame indexes
typedef struct{
	dsf2flac_uint64		offset;
	dsf2flac_uint32		length;
} DSTFrameIndex;

// this struct holds DST frame info
typedef struct {
	dsf2flac_uint32		numFrames;
	dsf2flac_uint16		frameRate;
	dsf2flac_uint32		frameSizeInSamplesPerChan;
	dsf2flac_uint32		frameSizeInBytesPerChan;
} DSTFrameInformation;



class dsdiffFileReader : public dsdSampleReader
{
public:
	// constructor and destructor
	dsdiffFileReader(char* filePath);
	virtual ~dsdiffFileReader();
public:
	// public overriden from dsdSampleReader
	bool step();
	void rewind();
	dsf2flac_int64 getLength() {return sampleCountPerChan;};
	dsf2flac_uint32 getNumChannels() {return chanNum;};
	dsf2flac_uint32 getSamplingFreq() {return samplingFreq;};
	bool msbIsYoungest() { return false;}
	bool samplesAvailable() { return !file.eof() && dsdSampleReader::samplesAvailable(); }; // false when no more samples left
	ID3_Tag getID3Tag(dsf2flac_uint32 trackNum);
	dsf2flac_uint32 getNumTracks() {return numTracks;}; // the number of audio tracks in the dsd data
	dsf2flac_uint64 getTrackStart(dsf2flac_uint32 trackNum);// return the index to the first sample of the nth track
	dsf2flac_uint64 getTrackEnd(dsf2flac_uint32 trackNum); // return the index to the first sample of the nth track
public:
	// other public methods
	void dispFileInfo();
private:
	// private variables
	fstreamPlus file;
	// read from the file - these are always present...
	dsf2flac_uint32 dsdiffVersion;
	dsf2flac_uint32 samplingFreq;
	dsf2flac_uint16 chanNum;
	dsf2flac_int8** chanIdents;
	dsf2flac_int8  compressionType[5];
	dsf2flac_int8* compressionName;
	DsdiffAst ast;
	dsf2flac_uint64 sampleDataPointer;
	dsf2flac_uint64 dstChunkEnd;
	dsf2flac_uint64 sampleCountPerChan;
	dsf2flac_uint16 lsConfig;
	// from optional chunks
	std::vector<DsdiffComment> comments;
	std::vector<ID3_Tag> tags;
	std::vector<DsdiffMarker> markers;
	bool isEm;
	char* emid;
	std::vector<DSTFrameIndex> dstFrameIndices;
	DSTFrameInformation dstInfo;
	// track info
	dsf2flac_uint32 numTracks;
	std::vector<dsf2flac_uint64> trackStartPositions;
	std::vector<dsf2flac_uint64> trackEndPositions;
	
	// vars to hold the data
	dsf2flac_uint8* sampleBuffer;
	dsf2flac_uint32 sampleBufferLenPerChan;
	dsf2flac_int64 bufferCounter; // stores the index to the current blockBuffer
	dsf2flac_int64 bufferMarker; // stores the current position in the blockBuffer
	// private methods
	void allocateSampleBuffer();
	bool readNextBlock();
	void processTracks();
	// all below here are for reading the headers
	bool readHeaders();
	static bool checkIdent(dsf2flac_int8* a, dsf2flac_int8* b); // MUST be used with the char[4]s or you'll get segfaults!
	bool readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart);
	bool readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart, dsf2flac_uint64 *chunkSz);
	// readers for specific types of chunk (return true if loaded ok)
	bool readChunk_FRM8(dsf2flac_uint64 chunkStart);
	bool readChunk_FVER(dsf2flac_uint64 chunkStart);
	bool readChunk_PROP(dsf2flac_uint64 chunkStart);
	bool readChunk_FS(dsf2flac_uint64 chunkStart);
	bool readChunk_CHNL(dsf2flac_uint64 chunkStart);
	bool readChunk_CMPR(dsf2flac_uint64 chunkStart);
	bool readChunk_ABSS(dsf2flac_uint64 chunkStart);
	bool readChunk_DSD(dsf2flac_uint64 chunkStart);
	bool readChunk_DST(dsf2flac_uint64 chunkStart);
	bool readChunk_DSTF(dsf2flac_uint64 chunkStart);
	bool readChunk_COMT(dsf2flac_uint64 chunkStart);
	bool readChunk_LSCO(dsf2flac_uint64 chunkStart);
	bool readChunk_ID3(dsf2flac_uint64 chunkStart);
	bool readChunk_DIIN(dsf2flac_uint64 chunkStart);
	bool readChunk_EMID(dsf2flac_uint64 chunkStart);
	bool readChunk_MARK(dsf2flac_uint64 chunkStart);
	bool readChunk_DSTI(dsf2flac_uint64 chunkStart);
	bool readChunk_FRTE(dsf2flac_uint64 chunkStart);
	
	void dispComment(DsdiffComment c);
	void dispMarker(DsdiffMarker m);
	
};

#endif // DSDIFFFILEREADER_H
