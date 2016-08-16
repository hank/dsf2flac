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
 
#ifndef DSDSAMPLEREADER_H
#define DSDSAMPLEREADER_H

#include <stdio.h>
#include <id3/tag.h>
#include <boost/circular_buffer.hpp>
#include "dsf2flac_types.h"

static const dsf2flac_uint32 defaultBufferLength = 5000; //!< The default length of the circular buffers.

/**
 * Abstract class defining anything which reads dsd samples from something.
 */
class DsdSampleReader
{
public:

	/// Constructor
	DsdSampleReader();
	/// Deconstructor
	virtual ~DsdSampleReader();

	/// Return false if the reader is invalid (format/file error for example).
	bool isValid();
	/// Returns a message explaining why the reader is invalid.
	std::string getErrorMsg();

	/// Returns the DSD sampling rate of this reader (Hz).
	virtual dsf2flac_uint32 getSamplingFreq() = 0;
	/// Returns the number of channels in the reader.
	virtual dsf2flac_uint32 getNumChannels() = 0;
	/// Returns the total number of DSD samples in the reader
	virtual dsf2flac_int64  getLength() = 0;
	/// Returns the total length of the reader in seconds.
	dsf2flac_float64 getLengthInSeconds();
	/// Returns the number of audio tracks in the reader
	virtual dsf2flac_uint32 getNumTracks() {return 1;};
	/// Returns the start position of the specified track.
	virtual dsf2flac_uint64 getTrackStart(dsf2flac_uint32 trackNum) { return 0; }
	/// Returns the end position of the specified track.
	virtual dsf2flac_uint64 getTrackEnd(dsf2flac_uint32 trackNum) { return getLength(); }

	/** Step the reader forward by 8 DSD samples.
	 *  This causes the next 8 DSD samples to be added into the front of the circular buffers (one uint8).
	 */
	virtual bool step() = 0;
	/// Returns false if there are no more samples left in the reader.
	virtual bool samplesAvailable() { return getPosition()<getLength(); };

	/// Return the current position of the reader in DSD samples.
	/// This is the position of the first entry in the circular buffers.
	dsf2flac_int64 getPosition() {return posMarker*samplesPerChar;};
	/// Return the current position of the reader in seconds.
	dsf2flac_float64 getPositionInSeconds();
	/// Return the current position of the reader as a percent of the total length.
	dsf2flac_float64 getPositionAsPercent();

	/// Set the reader position back to the start of the DSD data.
	/// Note that child classes implementing this method must call clearBuffer();
	virtual void rewind() = 0;

	/**
	 * Returns an array of circular buffers, one for each track.
	 * Each circular buffer contains getBufferLength() uint8 numbers.
	 * The DSD samples are packed into these uint8 numbers.
	 * The next uint8 set of 8 DSD samples is added into position 0 when step() is called.
	 * By default the buffer is filled with getIdleSample().
	 */
	boost::circular_buffer<dsf2flac_uint8>* getBuffer();
	/// Returns the length of the buffers (the number of uint8 numbers, NOT the number of DSD samples).
	dsf2flac_uint32 getBufferLength();
	/// Sets the length of the buffers (the number of uint8 numbers, NOT the number of DSD samples).
	/// Note that this will cause rewind() to be called.
	bool setBufferLength(dsf2flac_uint32 bufferLength);
	/// Describes the order that the samples are packed into the int8 buffer entries.
	virtual bool msbIsPlayedFirst() = 0;

	/// Return the ID3 tag of the first track in this reader.
	/// Useful for when there is only a single track in the reader.
	ID3_Tag getID3Tag() {return getID3Tag(1);};
	/// Return the ID3 tag corresponding to the provided track number.
	virtual ID3_Tag getID3Tag(dsf2flac_uint32 trackNum) {return NULL;};

	/// Return the idle tone used by this reader, by default the buffers are populated with this idle tone.
	virtual dsf2flac_uint8 getIdleSample() { return 0x69; };

	/// convert latin1 encoded char into utf8.
	static char* latin1_to_utf8(char* latin1);
protected:
	/// Allocates the circular buffers.
	/// Child classes need to call this once they know the number of channels!
	void allocateBuffer();
	/// Clear the buffers and fill with idleSample.
	void clearBuffer();
protected:
	// protected properties
	boost::circular_buffer<dsf2flac_uint8>* circularBuffers;
	// position marker
	dsf2flac_int64 posMarker; // implementors need to increment this on step()
	dsf2flac_uint32 samplesPerChar; // should be set by implementors
	// to hold feedback on errors
	bool valid;
	std::string errorMsg;
private:
	// private properties
	dsf2flac_uint32 bufferLength;
	bool isBufferAllocated;
};
#endif // DSDSAMPLEREADER_H
