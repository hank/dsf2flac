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


/**
 * This class extends dsdSampleReader providing access to dsf samples and other info
 * from dsdff files.
 *
 * Editied master files are supported, as is the undocumented ID3 chunk.
 * DST compression is also supported.
 */
class DsdiffFileReader : public DsdSampleReader
{
public:
	/** Class constructor.
	 *  filePath must be a valid dsdff file location.
	 *  If there is an issue reading or loading the file then isValid() will be false.
	 */
	DsdiffFileReader(char* filePath);
	/** Class destructor.
	 *  Closes the file and frees the internal buffers.
	 */
	virtual ~DsdiffFileReader();
public:
	// public overridden from dsdSampleReader
	bool step();
	void rewind();
	dsf2flac_int64 getLength() {return sampleCountPerChan;};
	dsf2flac_uint32 getNumChannels() {return chanNum;};
	dsf2flac_uint32 getSamplingFreq() {return samplingFreq;};
	bool msbIsPlayedFirst() { return false;}
	bool samplesAvailable() { return !file.eof() && DsdSampleReader::samplesAvailable(); }; // false when no more samples left
	ID3_Tag getID3Tag(dsf2flac_uint32 trackNum);
	dsf2flac_uint32 getNumTracks() {return numTracks;}; // the number of audio tracks in the dsd data
	dsf2flac_uint64 getTrackStart(dsf2flac_uint32 trackNum);// return the index to the first sample of the nth track
	dsf2flac_uint64 getTrackEnd(dsf2flac_uint32 trackNum); // return the index to the first sample of the nth track
public: // other public methods
	/// Can be called to display some useful info to stdout.
	void dispFileInfo();
private: // private methods
	/// Allocate the buffer to hold samples
	void allocateSampleBuffer();
	/// Read the next block of samples into the buffer.
	bool readNextBlock();
	/// Finds the number, start and end points of the tracks in the file.
	/// Must be called after the marker chunks have been read.
	void processTracks();
	// Called on create. Reads lots of info from the file.
	bool readHeaders();
	static bool checkIdent(dsf2flac_int8* a, dsf2flac_int8* b); // MUST be used with the char[4]s or you'll get segfaults!
	/// A little helper to read chunk headers info.
	bool readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart);
	/// A little helper to read chunk headers info.
	bool readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart, dsf2flac_uint64 *chunkSz);
	/// read data from a FRM8 chunk
	bool readChunk_FRM8(dsf2flac_uint64 chunkStart);
	/// read data from a FVER chunk
	bool readChunk_FVER(dsf2flac_uint64 chunkStart);
	/// read data from a PROP chunk
	bool readChunk_PROP(dsf2flac_uint64 chunkStart);
	/// read data from a FS chunk
	bool readChunk_FS(dsf2flac_uint64 chunkStart);
	/// read data from a CHNL chunk
	bool readChunk_CHNL(dsf2flac_uint64 chunkStart);
	/// read data from a CMPR chunk
	bool readChunk_CMPR(dsf2flac_uint64 chunkStart);
	/// read data from a ABSS chunk
	bool readChunk_ABSS(dsf2flac_uint64 chunkStart);
	/// read data from a DSD chunk
	bool readChunk_DSD(dsf2flac_uint64 chunkStart);
	/// read data from a DST chunk
	bool readChunk_DST(dsf2flac_uint64 chunkStart);
	/// read data from a DSTF chunk
	bool readChunk_DSTF(dsf2flac_uint64 chunkStart);
	/// read data from a COMT chunk
	bool readChunk_COMT(dsf2flac_uint64 chunkStart);
	/// read data from a LSCO chunk
	bool readChunk_LSCO(dsf2flac_uint64 chunkStart);
	/// read data from a ID3 chunk
	bool readChunk_ID3(dsf2flac_uint64 chunkStart);
	/// read data from a DIIN chunk
	bool readChunk_DIIN(dsf2flac_uint64 chunkStart);
	/// read data from a EMID chunk
	bool readChunk_EMID(dsf2flac_uint64 chunkStart);
	/// read data from a MARK chunk
	bool readChunk_MARK(dsf2flac_uint64 chunkStart);
	/// read data from a DSTI chunk
	bool readChunk_DSTI(dsf2flac_uint64 chunkStart);
	/// read data from a FRTE chunk
	bool readChunk_FRTE(dsf2flac_uint64 chunkStart);

	/// Can be called to display a DsdiffComment to stdout.
	void dispComment(DsdiffComment c);
	/// Can be called to display DsdiffMarker marker info to stdout.
	void dispMarker(DsdiffMarker m);
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
	
};

#endif // DSDIFFFILEREADER_H
