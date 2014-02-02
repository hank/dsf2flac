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
#include "dsdiff_file_reader.h"
#include "iostream"
#include "libdstdec/dst_init.h"
#include "libdstdec/dst_fram.h"
static bool chanIdentsAllocated = false;
static bool sampleBufferAllocated = false;
static ebunch dstEbunch;
static bool dstEbunchAllocated = false;
dsdiffFileReader::dsdiffFileReader(char* filePath) : dsdSampleReader()
{
	// set some defaults
	ast.hours = 0;
	ast.minutes = 0;
	ast.seconds = 0;
	ast.samples = 0;
	samplesPerChar = 8; // always 8 samples per char (dsd wide not supported by dsdiff).
	sampleBufferLenPerChan = 1024;
	bufferMarker = 0;
	errorMsg = "";
	lsConfig=65535;
	isEm=false;
	// first let's open the file
	file.open(filePath, fstreamPlus::in | fstreamPlus::binary);
	// throw exception if that did not work.
	if (!file.is_open()) {
		errorMsg = "could not open file";
		valid = false;
		return;
	}
	// try and read the header data
	if (!(valid = readHeaders()))
		return;
		
	// find the start and end of the tracks.
	processTracks();
		
	// if DST data, then initialise the decoder
	if (checkIdent(compressionType,const_cast<dsf2flac_int8*>("DST "))) {
		DST_InitDecoder(&dstEbunch, getNumChannels(), getSamplingFreq()/44100);
		dstEbunchAllocated = true;
	}
	
	rewind(); // calls allocateBlockBuffer
	
	return;
}
dsdiffFileReader::~dsdiffFileReader()
{
	// close the file
	file.close();
	// free mem in the chanIdents
	if (chanIdentsAllocated) {
		for (dsf2flac_uint16 i=0; i<chanNum; i++)
			delete[] chanIdents[i];
		delete[] chanIdents;
	}
	// free sample buffer
	if (sampleBufferAllocated)
		delete[] sampleBuffer;
	// free comments
	typename std::vector<DsdiffComment>::iterator c=comments.begin();
	while(c!=comments.end()) {
		delete[] c->commentText;
		++c;
	}
	comments.clear();
	// free markers
	typename std::vector<DsdiffMarker>::iterator m=markers.begin();
	while(m!=markers.end()) {
		delete[] m->markerText;
		++m;
	}
	markers.clear();
	// free emid
	if (isEm)
		delete[] emid;
	
	// free the DST decoder (assuming one was used)
	if (dstEbunchAllocated) {
		DST_CloseDecoder(&dstEbunch);
	}
}
/**
 * void dsdiffFileReader::allocateSampleBuffer()
 *
 * allocate the buffer to hold samples
 *
 */
void dsdiffFileReader::allocateSampleBuffer()
{
	if (sampleBufferAllocated)
		return;
	sampleBuffer = new dsf2flac_uint8[getNumChannels()*sampleBufferLenPerChan];
	sampleBufferAllocated = true;
}
/**
 * void dsdiffFileReader::rewind()
 *
 * Reset the position back to the start of the file
 *
 */
void dsdiffFileReader::rewind()
{
	// position the file at the start of the data chunk
	if (file.seekg(sampleDataPointer)) {
		errorMsg = "dsfFileReader::rewind:file seek error";
	}
	allocateSampleBuffer();
	bufferCounter = 0;
	bufferMarker = 0;
	readNextBlock();
	bufferCounter = 0;
	posMarker = -1;
	clearBuffer();
}

/**
 * bool dsdiffFileReader::readNextBlock()
 *
 * read a block of samples into the buffer.
 *
 */
bool dsdiffFileReader::readNextBlock() {
	
	bool ok = true;
	// return false if this is the end of the file
	if (!samplesAvailable()) {
		errorMsg = "dsfFileReader::readNextBlock:no more data in file";
		ok = false;
	}
	
	dsf2flac_int64 samplesLeft = getLength()-getPosition();
	
	if (ok && checkIdent(compressionType,const_cast<dsf2flac_int8*>("DSD "))) {
		if (samplesLeft/samplesPerChar < sampleBufferLenPerChan) {
			// fill the blockBuffer with the idle sample
			for (dsf2flac_uint32 i=0; i<chanNum*sampleBufferLenPerChan; i++)
				sampleBuffer[i] = getIdleSample();
			if (file.read_uint8(sampleBuffer,chanNum*samplesLeft/samplesPerChar)) {
				errorMsg = "dsfFileReader::readNextBlock:file read error";
				ok = false;
			}
		} else if (file.read_uint8(sampleBuffer,chanNum*sampleBufferLenPerChan)) {
			errorMsg = "dsfFileReader::readNextBlock:file read error";
			ok = false;
		}
	} else if (ok && checkIdent(compressionType,const_cast<dsf2flac_int8*>("DST "))) {
		
		dsf2flac_uint64 chunkStart = file.tellg();
		dsf2flac_uint64 chunkSz;
		dsf2flac_int8 ident[5];
		ident[4]='\0';
		
		if (chunkStart > dstChunkEnd)
			ok = false;
		
		// read the chunk header
		if (ok)
			ok = readChunkHeader(ident,chunkStart,&chunkSz);
		
		// we might have a DSTC chunk, which we will ignore
		if (ok)
			if (checkIdent(ident,const_cast<dsf2flac_int8*>("DSTC")))
				ok = readChunkHeader(ident,chunkStart,&chunkSz);
				
		// decode
		if (ok)
			ok = readChunk_DSTF(chunkStart);

		// make sure we are in the right place for next time
		file.seekg(chunkStart + chunkSz);
	} else
		ok = false;
	
	
	if (!ok) {
		// fill the blockBuffer with the idle sample
		for (dsf2flac_uint32 i=0; i<chanNum*sampleBufferLenPerChan; i++)
			sampleBuffer[i] = getIdleSample();
	}
	
	bufferCounter++;
	bufferMarker=0;

	return ok;
}
/**
 * void dsdiffFileReader::step()
 *
 * Increments the position in the file by 8 dsd samples (1 byte of data).
 * The block buffers are updated with the new samples.
 *
 */
bool dsdiffFileReader::step()
{
	bool ok = true;
	
	if (!samplesAvailable())
		ok = false;
	else if (bufferMarker>=sampleBufferLenPerChan)
		ok = readNextBlock();
	
	if (ok) {
		for (dsf2flac_uint16 i=0; i<chanNum; i++)
			circularBuffers[i].push_front(sampleBuffer[i+bufferMarker*chanNum]);
		bufferMarker++;
	} else {
		for (dsf2flac_uint16 i=0; i<chanNum; i++)
			circularBuffers[i].push_front(getIdleSample());
	}
	
	posMarker++;
	return ok;
}
/** 
 * dsf2flac_uint64 dsdiffFileReader::getTrackStart(dsf2flac_uint32 trackNum);
 * 
 * return the index to the first sample of the nth track
 */
dsf2flac_uint64 dsdiffFileReader::getTrackStart(dsf2flac_uint32 trackNum) {
	if (trackNum >= numTracks)
		return 0;
	return trackStartPositions[trackNum];
}
/** 
 * dsf2flac_uint64 dsdiffFileReader::getTrackEnd(dsf2flac_uint32 trackNum);
 * 
 * return the index to the last sample of the nth track
 */
dsf2flac_uint64 dsdiffFileReader::getTrackEnd(dsf2flac_uint32 trackNum) {
	if (trackNum >= numTracks)
		return getLength();
	return trackEndPositions[trackNum];
		
}
/** 
 * ID3_Tag dsdiffFileReader::getID3Tag(dsf2flac_uint32 trackNum);
 * 
 * Public function, returns id3tags for the specified track
 * or NULL if there is no such tag.
 */
 ID3_Tag dsdiffFileReader::getID3Tag(dsf2flac_uint32 trackNum) {
	if (trackNum >= tags.size())
		return NULL;
	return tags[trackNum];
}
/**
 * void dsdiffFileReader::readHeaders()
 *
 * Private function, called on create. Reads lots of info from the file.
 *
 */
bool dsdiffFileReader::readHeaders()
{
	// look for the FRM8 chunk (probably at the start of the data).
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkStart;
	dsf2flac_uint64 chunkSz;
	bool frm8read = false;
	bool frm8valid = false;
	// assum first chunk is at the start of the file.
	chunkStart = 0;
	// stop once one frm8 chunk is read
	while (!frm8read) {
		// read the chunk header
		if (!readChunkHeader(ident,chunkStart,&chunkSz))
			return false;
		// did we find a FRM8 chunk??
		if ( checkIdent(ident,const_cast<dsf2flac_int8*>("FRM8")) ) {
			frm8valid = readChunk_FRM8(chunkStart);
			frm8read = true;
		}
		// otherwise, move to the next chunk
		chunkStart +=chunkSz;
	}
	return frm8valid;
}
bool dsdiffFileReader::readChunk_FRM8(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("FRM8")) ) {
		errorMsg = "dsdiffFileReader::readChunk_FRM8:chunk ident error";
		return false;
	}
	// another ident which MUST be "DSD "
	if (file.read_int8_rev(ident,4)) {
		errorMsg = "dsdiffFileReader::readChunk_FRM8:file read error";
		return false;
	}
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("DSD ")) ) {
		errorMsg = "dsdiffFileReader::readChunk_FRM8:chunk ident error";
		return false;
	}
	// read all sub chunks in the FRM8 chunk
	dsf2flac_uint64 subChunkStart = file.tellg();
	dsf2flac_uint64 subChunkSz;
	// keep track of the non-optional chunks
	bool found_fver = false;
	bool found_prop = false;
	bool found_dsdt = false; // either dsd or dst
	
	while (subChunkStart < chunkStart + chunkSz) {
		// read the header
		if (!readChunkHeader(ident,subChunkStart,&subChunkSz))
			return false;
		// see if we know how to read this chunk
		if ( !found_fver && checkIdent(ident,const_cast<dsf2flac_int8*>("FVER")) ) {
			found_fver = readChunk_FVER(subChunkStart);
		} else if ( !found_prop && checkIdent(ident,const_cast<dsf2flac_int8*>("PROP")) ) {
			found_prop = readChunk_PROP(subChunkStart);
		} else if ( !found_dsdt && checkIdent(ident,const_cast<dsf2flac_int8*>("DSD ")) ) {
			found_dsdt = readChunk_DSD(subChunkStart);
		} else if ( !found_dsdt && checkIdent(ident,const_cast<dsf2flac_int8*>("DST ")) ) {
			found_dsdt = readChunk_DST(subChunkStart);
		} else if ( checkIdent(ident,const_cast<dsf2flac_int8*>("COMT")) ) {
			readChunk_COMT(subChunkStart);
		} else if ( checkIdent(ident,const_cast<dsf2flac_int8*>("ID3 ")) ) {
			readChunk_ID3(subChunkStart);
		} else if ( checkIdent(ident,const_cast<dsf2flac_int8*>("DIIN")) ) {
			readChunk_DIIN(subChunkStart);
		} else if ( checkIdent(ident,const_cast<dsf2flac_int8*>("DSTI")) ) {
			readChunk_DSTI(subChunkStart);
		} else
			printf("WARNING: unknown chunk type: %s\n",ident);
		// move to the next chunk
		subChunkStart = subChunkStart + subChunkSz;
	}
	// return true if all required chunks are ok.
	if (!found_fver) {
		errorMsg = "dsdiffFileReader::readChunk_FRM8:No valid FVER chunk found";
		return false;
	} else if (!found_prop) {
		errorMsg = "dsdiffFileReader::readChunk_FRM8:No valid PROP chunk found";
		return false;
	} else if (!found_dsdt) {
		errorMsg = "dsdiffFileReader::readChunk_FRM8:No valid DSD or DST chunk found";
		return false;
	} else
		return true;
}
bool dsdiffFileReader::readChunk_FVER(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("FVER")) ) {
		errorMsg = "dsdiffFileReader::readChunk_FVER:chunk ident error";
		return false;
	}
	// read the file version
	if (file.read_uint32_rev(&dsdiffVersion,1)) {
		errorMsg = "dsdiffFileReader::readChunk_FVER:file read error";
		return false;
	};
	return true;
}
bool dsdiffFileReader::readChunk_PROP(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("PROP")) ) {
		errorMsg = "dsdiffFileReader::readChunk_PROP:chunk ident error";
		return false;
	}
	// another ident which MUST be "SND "
	if (file.read_int8(ident,4)) {
		errorMsg = "dsdiffFileReader::readChunk_PROP:file read error";
		return false;
	}
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("SND ")) ) {
		errorMsg = "dsdiffFileReader::readChunk_PROP:PROP chunk format error";
		return false;
	}
	// read all sub chunks in the FRM8 chunk
	dsf2flac_uint64 subChunkStart = file.tellg();
	dsf2flac_uint64 subChunkSz;
	// keep track of the chunks we've read
	bool found_fs   = false;
	bool found_chnl = false;
	bool found_cmpr = false;
	bool found_abss = false;
	bool found_lsco = false;
	//
	while (subChunkStart < chunkStart + chunkSz) {
		// read the header
		if (!readChunkHeader(ident,subChunkStart,&subChunkSz))
			return false;
		// see if we know how to read this chunk
		if ( !found_fs && checkIdent(ident,const_cast<dsf2flac_int8*>("FS  ")) ) {
			found_fs = readChunk_FS(subChunkStart);
		} else if ( !found_chnl && checkIdent(ident,const_cast<dsf2flac_int8*>("CHNL")) ) {
			found_chnl = readChunk_CHNL(subChunkStart);
		} else if ( !found_cmpr && checkIdent(ident,const_cast<dsf2flac_int8*>("CMPR")) ) {
			found_cmpr = readChunk_CMPR(subChunkStart);
		} else if ( !found_abss && checkIdent(ident,const_cast<dsf2flac_int8*>("ABSS")) ) {
			found_abss = readChunk_ABSS(subChunkStart);
		} else if ( !found_lsco && checkIdent(ident,const_cast<dsf2flac_int8*>("LSCO")) ) {
			found_lsco = readChunk_LSCO(subChunkStart);
		} else
			printf("WARNING: unknown or repeated chunk type: %s\n",ident);
		// move to the next chunk
		subChunkStart = subChunkStart + subChunkSz;
	}
	// check that all the required chunks were read
	if (!found_fs) {
		errorMsg = "dsdiffFileReader::readChunk_PROP: no valid FS chunk";
		return false;
	} else if (!found_chnl) {
		errorMsg = "dsdiffFileReader::readChunk_PROP: no valid CHNL chunk";
		return false;
	} else if (!found_cmpr) {
		errorMsg = "dsdiffFileReader::readChunk_PROP: no valid CMPR chunk";
		return false;
	} else
		return true;
}
bool dsdiffFileReader::readChunk_FS(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("FS  ")) ) {
		errorMsg = "dsdiffFileReader::readChunk_FS:chunk ident error";
		return false;
	}
	if (file.read_uint32_rev(&samplingFreq,1)) {
		errorMsg = "dsdiffFileReader::readChunk_FS:File read error";
		return false;
	}
	return true;
}
bool dsdiffFileReader::readChunk_CHNL(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("CHNL")) ) {
		errorMsg = "dsdiffFileReader::readChunk_CHNL:chunk ident error";
		return false;
	}
	// read number of channels
	if (file.read_uint16_rev(&chanNum,1)) {
		errorMsg = "dsdiffFileReader::readChunk_CHNL:file read error";
		return false;
	}
	// read channel identifiers
	chanIdents = new dsf2flac_int8*[chanNum];
	for (dsf2flac_uint16 i=0; i<chanNum; i++) {
		chanIdents[i] = new dsf2flac_int8[5];
		if (file.read_int8(chanIdents[i],4)) {
			errorMsg = "dsdiffFileReader::readChunk_CHNL:file read error";
			return false;
		};
		chanIdents[i][4] = '\0';
	}
	return true;
}
bool dsdiffFileReader::readChunk_CMPR(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("CMPR")) ) {
		errorMsg = "dsdiffFileReader::readChunk_CMPR:chunk ident error";
		return false;
	}
	// type of compression
	compressionType[4] = '\0';
	if (file.read_int8(compressionType,4)) {
		errorMsg = "dsdiffFileReader::readChunk_CMPR:file read error";
		return false;
	}
	// read length of next entry
	dsf2flac_uint8 n;
	if (file.read_uint8(&n,1)) {
		errorMsg = "dsdiffFileReader::readChunk_CMPR:file read error";
		return false;
	}
	compressionName = new dsf2flac_int8[n+1];
	compressionName[n] = '\0';
	if (file.read_int8(compressionName,n)) {
		errorMsg = "dsdiffFileReader::readChunk_CMPR:file read error";
		return false;
	}
	return true;
}
bool dsdiffFileReader::readChunk_ABSS(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("ABSS")) ) {
		errorMsg = "dsdiffFileReader::readChunk_ABSS:chunk ident error";
		return false;
	}
	// hours
	if (file.read_uint16_rev(&ast.hours,1)) {
		errorMsg = "dsdiffFileReader::readChunk_ABSS:file read error";
		return false;
	}
	// mins
	if (file.read_uint8(&ast.minutes,1)) {
		errorMsg = "dsdiffFileReader::readChunk_ABSS:file read error";
		return false;
	}
	// secs
	if (file.read_uint8(&ast.seconds,1)) {
		errorMsg = "dsdiffFileReader::readChunk_ABSS:file read error";
		return false;
	}
	// samples
	if (file.read_uint32_rev(&ast.samples,1)) {
		errorMsg = "dsdiffFileReader::readChunk_ABSS:file read error";
		return false;
	}
	return true;
}
bool dsdiffFileReader::readChunk_ID3(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("ID3 ")) ) {
		errorMsg = "dsdiffFileReader::readChunk_ID3:chunk ident error";
		return false;
	}
	// read the first ID3_TAGHEADERSIZE bytes of the metadata (which should be the header).
	dsf2flac_uint8 id3header[ID3_TAGHEADERSIZE];
	if (file.read_uint8(id3header,ID3_TAGHEADERSIZE)) {
		return false;
	}
	// check this is actually an id3 header
	dsf2flac_uint64 id3tagLen;
	if ( (id3tagLen = ID3_IsTagHeader(id3header)) > -1 )
		return false;
	// read the tag
	dsf2flac_uint8* id3tag = new dsf2flac_uint8[ id3tagLen ];
	if (file.read_uint8(id3tag,id3tagLen)) {
		return false;
	}
	ID3_Tag t;
	t.Parse (id3header, id3tag);
	tags.push_back(t);
	delete[] id3tag;
	return true;
}
bool dsdiffFileReader::readChunk_LSCO(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("LSCO")) ) {
		errorMsg = "dsdiffFileReader::readChunk_LSCO:chunk ident error";
		return false;
	}
	if (file.read_uint16(&lsConfig,1)) {
		errorMsg = "dsdiffFileReader::readChunk_LSCO:file read error";
		return false;
	}
	return true;
}
bool dsdiffFileReader::readChunk_DIIN(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("DIIN")) ) {
		errorMsg = "dsdiffFileReader::readChunk_DIIN:chunk ident error";
		return false;
	}
	// read all sub chunks in the FRM8 chunk
	dsf2flac_uint64 subChunkStart = file.tellg();
	dsf2flac_uint64 subChunkSz;
	bool emidRead = false;
	while (subChunkStart < chunkStart + chunkSz) {
		// read the header
		if (!readChunkHeader(ident,subChunkStart,&subChunkSz))
			return false;
		// see if we know how to read this chunk
		if ( !emidRead && checkIdent(ident,const_cast<dsf2flac_int8*>("EMID")) ) {
			emidRead = readChunk_EMID(subChunkStart);
		} else if ( checkIdent(ident,const_cast<dsf2flac_int8*>("MARK")) ) {
			readChunk_MARK(subChunkStart);
		} else
			printf("WARNING: unknown chunk type: %s\n",ident);
		// move to the next chunk
		subChunkStart = subChunkStart + subChunkSz;
	}
	return true;
}
bool dsdiffFileReader::readChunk_EMID(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("EMID")) ) {
		errorMsg = "dsdiffFileReader::readChunk_EMID:chunk ident error";
		return false;
	}
	isEm = true;
	dsf2flac_uint64 n = chunkSz - 12;
	emid = new char[n + 1];
	emid[n] = '\0';
	if (file.read_int8(emid,n)) {
		errorMsg = "dsdiffFileReader::readChunk_EMID:file read error";
		return false;
	}
	return true;
}
bool dsdiffFileReader::readChunk_DSTI(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("DSTI")) ) {
		errorMsg = "dsdiffFileReader::readChunk_DSTI:chunk ident error";
		return false;
	}
	dsf2flac_uint64 n = (chunkSz - 12)/(32+64);
	for (dsf2flac_uint64 i=0; i<n; i++) {
		DSTFrameIndex in;
		if (file.read_uint64_rev(&in.offset,1)) {
			errorMsg = "dsdiffFileReader::readChunk_DSTI:file read error";
			return false;
		}
		if (file.read_uint32_rev(&in.length,1)) {
			errorMsg = "dsdiffFileReader::readChunk_DSTI:file read error";
			return false;
		}
		dstFrameIndices.push_back(in);
	}
	return true;
}
bool dsdiffFileReader::readChunk_MARK(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("MARK")) ) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:chunk ident error";
		return false;
	}
	DsdiffMarker m;
	if (file.read_uint16_rev(&m.hours,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint8_rev(&m.minutes,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint8_rev(&m.seconds,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint32_rev(&m.samples,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_int32_rev(&m.offset,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint16_rev(&m.markType,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint16_rev(&m.markChannel,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint16_rev(&m.trackFlags,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (file.read_uint32_rev(&m.count,1)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	m.markerText = new dsf2flac_int8[m.count+1];
	m.markerText[m.count] = '\0';
	if (file.read_int8_rev(m.markerText,m.count)) {
		errorMsg = "dsdiffFileReader::readChunk_MARK:file read error";
		return false;
	}
	if (m.count % 2)
		file.seekg(1,fstreamPlus::cur);
	markers.push_back(m);
	//dispMarker(m);
	return true;
}


dsf2flac_uint64 decodeMarkerPosition(DsdiffAst ast, DsdiffMarker marker, dsf2flac_uint32 fs) {
	dsf2flac_int64 dhr = (dsf2flac_int64) marker.hours - (dsf2flac_int64) ast.hours;
	dsf2flac_int64 dmi = (dsf2flac_int64) marker.minutes - (dsf2flac_int64) ast.minutes;
	dsf2flac_int64 dse = (dsf2flac_int64) marker.seconds - (dsf2flac_int64) ast.seconds;
	dsf2flac_int64 dsa = (dsf2flac_int64) marker.samples - (dsf2flac_int64) ast.samples;
	dsf2flac_int64 pos = ( dhr*60*60 + dmi*60 + dse ) * fs + dsa;
	pos += marker.offset;
	return pos; // note div by 8 to give position in chars
}


void dsdiffFileReader::processTracks() {
	
	numTracks = 0;
	dsf2flac_uint64 thisTrackStart = 0;
	bool inTrack = false;
	
	for (unsigned int i = 0; i < markers.size(); i++) {
		
		// we only care about track markers
		if (markers[i].markType != 0 && markers[i].markType != 1)
			continue;
			
		// find the position of this marker in chars
		dsf2flac_uint64 pos = decodeMarkerPosition(ast,markers[i],samplingFreq);
		
		// if we are inTrack then this marker must denote the end of the current track
		if (inTrack) {
			// add the track
			numTracks ++;
			trackStartPositions.push_back(thisTrackStart);
			trackEndPositions.push_back(pos - 1);
			inTrack = false;
		}
		
		// if track start maker record pos and flag that we are inTrack.
		if (markers[i].markType == 0) {
			thisTrackStart = pos;
			inTrack = true;
		}
	}
	
	// if there are no markers then make a sensible default
	if ( numTracks ==0 ) {
		numTracks = 1;
		trackStartPositions.push_back(0);
		trackEndPositions.push_back(getLength());
	}
	
	return;
}
void dsdiffFileReader::dispMarker(DsdiffMarker m)
{
	printf("\nmarkType:\t%u\n",m.markType);
	printf("time:\t%u:%u:%u::%u + %u\n",m.hours,m.minutes,m.seconds,m.samples,m.offset);
	printf("markChannel:\t\t%u\n",m.markChannel);
	printf("trackFlags:\t\t%u\n",m.trackFlags);
	printf("markerText:\t%s\n",m.markerText);
	//printf("count:\t\t%u\n",m.count);
}
bool dsdiffFileReader::readChunk_COMT(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	if (!readChunkHeader(ident,chunkStart))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("COMT")) ) {
		errorMsg = "dsdiffFileReader::readChunk_COMT:chunk ident error";
		return false;
	}
	dsf2flac_uint16 numComments;
	if (file.read_uint16_rev(&numComments,1)) {
		errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
		return false;
	}
	for (short unsigned int i=0; i<numComments; i++) {
		DsdiffComment c;
		if (file.read_uint16_rev(&c.timeStampYear,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint8_rev(&c.timeStampMonth,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint8_rev(&c.timeStampDay,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint8_rev(&c.timeStampHour,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint8_rev(&c.timeStampMinutes,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint16_rev(&c.cmtType,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint16_rev(&c.cmtRef,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (file.read_uint32_rev(&c.count,1)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		c.commentText = new dsf2flac_int8[c.count+1];
		c.commentText[c.count] = '\0';
		if (file.read_int8_rev(c.commentText,c.count)) {
			errorMsg = "dsdiffFileReader::readChunk_COMT:file read error";
			return false;
		}
		if (c.count % 2)
			file.seekg(1,fstreamPlus::cur);
		comments.push_back(c);
	}
	return true;
}
void dsdiffFileReader::dispComment(DsdiffComment c)
{
	printf("timeStampYear:\t%u\n",c.timeStampYear);
	printf("timeStampMonth:\t%u\n",c.timeStampMonth);
	printf("timeStampDay:\t%u\n",c.timeStampDay);
	printf("timeStampHour:\t%u\n",c.timeStampHour);
	printf("timeStampMinutes:\t%u\n",c.timeStampMinutes);
	printf("cmtType:\t%u\n",c.cmtType);
	printf("cmtRef:\t\t%u\n",c.cmtRef);
	printf("count:\t\t%u\n",c.count);
	printf("commentText:\t%s\n",c.commentText);
}
bool dsdiffFileReader::readChunk_DSD(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkLen;
	if (!readChunkHeader(ident,chunkStart,&chunkLen))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("DSD ")) ) {
		errorMsg = "dsdiffFileReader::readChunk_DSD:chunk ident error";
		return false;
	}
	// we are now at the start of the dsd data, record the position
	sampleDataPointer = file.tellg();
	// chunkLen contains the number of samples
	sampleCountPerChan = (chunkLen - 12)/chanNum*samplesPerChar;
	return true;
}
bool dsdiffFileReader::readChunk_DSTF(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkLen;
	if (!readChunkHeader(ident,chunkStart,&chunkLen))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("DSTF")) ) {
		errorMsg = "dsdiffFileReader::readChunk_DSTF:chunk ident error";
		return false;
	}
	dsf2flac_uint64 dst_framesize = chunkLen-12;
	dsf2flac_uint8* dst_data = new dsf2flac_uint8[dst_framesize];
	if (file.read_uint8_rev(dst_data,dst_framesize)) {
		errorMsg = "dsdiffFileReader::readChunk_DSTF:file read error";
		return false;
	}
	
	if (DST_FramDSTDecode(dst_data, sampleBuffer,dst_framesize, dstInfo.numFrames, &dstEbunch))
		return false;
	
	return true;
}
bool dsdiffFileReader::readChunk_DST(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("DST ")) ) {
		errorMsg = "dsdiffFileReader::readChunk_DST:chunk ident error";
		return false;
	}
	// record the end of the chunk
	dstChunkEnd = chunkStart + chunkSz -1;
	// read all sub chunks in the DST chunk
	dsf2flac_uint64 subChunkStart = file.tellg();
	dsf2flac_uint64 subChunkSz;
	bool found_frte = false;
	bool found_dstf = false;
	//
	while (!(found_frte && found_dstf) && subChunkStart < chunkStart + chunkSz) {
		// read the header
		if (!readChunkHeader(ident,subChunkStart,&subChunkSz))
			return false;
		// see if we know how to read this chunk
		if ( !found_frte && checkIdent(ident,const_cast<dsf2flac_int8*>("FRTE")) ) {
			found_frte = readChunk_FRTE(subChunkStart);
		} else if ( !found_dstf && checkIdent(ident,const_cast<dsf2flac_int8*>("DSTF")) ) {
			found_dstf = true;
			sampleDataPointer = subChunkStart;
		} else
			printf("WARNING: unknown chunk type: %s\n",ident);
		// move to the next chunk
		subChunkStart = subChunkStart + subChunkSz;
	}
	if (found_frte) {
		sampleCountPerChan = (dsf2flac_uint64)dstInfo.numFrames * (dsf2flac_uint64)dstInfo.frameSizeInSamplesPerChan;
		sampleBufferLenPerChan = dstInfo.frameSizeInBytesPerChan;
	}
	return found_frte && found_dstf;
}
bool dsdiffFileReader::readChunk_FRTE(dsf2flac_uint64 chunkStart)
{
	// read the header so that we are certain we are in the correct place.
	dsf2flac_int8 ident[5];
	ident[4]='\0';
	dsf2flac_uint64 chunkSz;
	if (!readChunkHeader(ident,chunkStart,&chunkSz))
		return false;
	// check the ident
	if ( !checkIdent(ident,const_cast<dsf2flac_int8*>("FRTE")) ) {
		errorMsg = "dsdiffFileReader::readChunk_FRTE:chunk ident error";
		return false;
	}
	if (file.read_uint32_rev(&dstInfo.numFrames,1)) {
		errorMsg = "dsdiffFileReader::readChunk_FRTE:file read error";
		return false;
	}
	if (file.read_uint16_rev(&dstInfo.frameRate,1)) {
		errorMsg = "dsdiffFileReader::readChunk_FRTE:file read error";
		return false;
	}
	dstInfo.frameSizeInSamplesPerChan = getSamplingFreq()/dstInfo.frameRate;
	dstInfo.frameSizeInBytesPerChan = dstInfo.frameSizeInSamplesPerChan/samplesPerChar;
	return true;
}
/**
 * bool dsdiffFileReader::readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart)
 *
 * Private method: a little helper to get rid of repetitive code!
 *
 */
bool dsdiffFileReader::readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart)
{
	dsf2flac_uint64 chunkSz;
	return readChunkHeader(ident,chunkStart,&chunkSz);
}
bool dsdiffFileReader::readChunkHeader(dsf2flac_int8* ident, dsf2flac_uint64 chunkStart, dsf2flac_uint64* chunkSz)
{
	// make sure we are at the start of the chunk
	if (file.seekg(chunkStart)) {
		errorMsg = "dsdiffFileReader::readChunkHeader:File seek error";
		return false;
	}
	// read the ident
	if (file.read_int8(ident,4)) {
		errorMsg = "dsdiffFileReader::readChunkHeader:File read error";
		return false;
	}
	// 8 bytes chunk size
	if (file.read_uint64_rev(chunkSz,1)) {
		errorMsg = "dsdiffFileReader::readChunkHeader:File read error";
		return false;
	}
	//printf("%s\n",ident);
	// if the chunk size is odd then add one... because for some stupid reason you have to do this....
	// also add the header length (12bytes).
	if ( chunkSz[0] & 1 )
		chunkSz[0] += 13;
	else
		chunkSz[0] += 12;
	return true;
}
bool dsdiffFileReader::checkIdent(dsf2flac_int8* a, dsf2flac_int8* b)
{
	return ( a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3] );
}
void dsdiffFileReader::dispFileInfo()
{
	printf("dsdiffVersion: %08x\n",dsdiffVersion);
	printf("samplingRate: %u\n",samplingFreq);
	printf("chanNum: %u\n",chanNum);
	for (int i=0; i<chanNum; i++)
		printf("chanIdent%u: %s\n",i,chanIdents[i]);
	printf("compressionType: %s\n",compressionType);
	printf("compressionName: %s\n",compressionName);
	printf("ast_hours: %u\n",ast.hours);
	printf("ast_mins: %d\n",ast.minutes);
	printf("ast_secs: %d\n",ast.seconds);
	printf("ast_samples: %u\n",ast.samples);
	printf("sampleDataPointer: %lu\n",sampleDataPointer);
	printf("sampleCount: %lu\n",sampleCountPerChan);
}
