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

#include "dsd_sample_reader.h"

DsdSampleReader::DsdSampleReader()
{
	bufferLength = defaultBufferLength;
	isBufferAllocated = false;
}

DsdSampleReader::~DsdSampleReader()
{
	if (isBufferAllocated)
		delete[] circularBuffers;
	isBufferAllocated = false;
}

boost::circular_buffer<dsf2flac_uint8>* DsdSampleReader::getBuffer()
{
	return circularBuffers;
}

dsf2flac_uint32 DsdSampleReader::getBufferLength()
{
	return bufferLength;
}

bool DsdSampleReader::setBufferLength(dsf2flac_uint32 b)
{
	if (b<1) {
		errorMsg = "dsdSampleReader::setBufferLength:buffer length must be >0";
		return false;
	}
	bufferLength=b;
	if (!isBufferAllocated)
		allocateBuffer();
	for (dsf2flac_uint32 i = 0; i<getNumChannels(); i++)
		circularBuffers[i].set_capacity(getBufferLength());
	clearBuffer(); // should be called by rewind... but just incase.
	rewind();
	return true;
}

dsf2flac_float64 DsdSampleReader::getPositionInSeconds()
{
	return getPosition() / (dsf2flac_float64) getSamplingFreq();
}

dsf2flac_float64 DsdSampleReader::getPositionAsPercent()
{
	return 100* getPosition() / (dsf2flac_float64) getLength();
}

dsf2flac_float64 DsdSampleReader::getLengthInSeconds()
{
	return getLength() / (dsf2flac_float64) getSamplingFreq();
}

bool DsdSampleReader::isValid()
{
	return valid;
}

std::string DsdSampleReader::getErrorMsg()
{
	return errorMsg;
}

void DsdSampleReader::allocateBuffer()
{
	if (isBufferAllocated)
		return;
		
	circularBuffers = new boost::circular_buffer<dsf2flac_uint8> [getNumChannels()];
	for (dsf2flac_uint32 i = 0; i<getNumChannels(); i++) {
		boost::circular_buffer<dsf2flac_uint8> cb(getBufferLength());
		circularBuffers[i] = cb;
	}
	isBufferAllocated = true;
	clearBuffer();
	return;
}

void DsdSampleReader::clearBuffer() 
{
	if (!isBufferAllocated) {
		allocateBuffer();
		return;
	}
	
	dsf2flac_uint8 c = getIdleSample();
	for (dsf2flac_uint32 i = 0; i<getNumChannels(); i++)
		for (dsf2flac_uint32 j=0; j<getBufferLength(); j++)
			circularBuffers[i].push_front(c);
}

