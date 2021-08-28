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
 
#include "dsd_decimator.h"
#include <cmath>
#include "filters.cpp"

static bool lookupTableAllocated = false;

DsdDecimator::DsdDecimator(DsdSampleReader *r, dsf2flac_uint32 rate)
{
	reader = r;
	outputSampleRate = rate;
	valid = true;;
	errorMsg = "";
	
	// ratio of out to in sampling rates
	ratio = r->getSamplingFreq() / outputSampleRate;
	// how many bytes to skip after each out sample calc.
	nStep = ratio/8; 
	
	// load the required filter into the lookuptable based on in and out sample rate
	if (ratio == 8)
		initLookupTable(nCoefs_352,coefs_352,tzero_352);
	else if (ratio == 16)
		initLookupTable(nCoefs_176_sa,coefs_176_sa,tzero_176_sa);
	else if (ratio == 32)
		initLookupTable(nCoefs_88,coefs_88,tzero_88);
	else
	{
		valid = false;
		errorMsg = "Sorry, incompatible sample rate combination";
		return;
	}
	// set the buffer to the length of the table if not long enough
	if (nLookupTable > reader->getBufferLength())
		reader->setBufferLength(nLookupTable);
}

DsdDecimator::~DsdDecimator()
{
	if (lookupTableAllocated) {
		for (dsf2flac_uint32 n=0; n<nLookupTable; n++)
		{
			delete[] lookupTable[n];
		}
		delete[] lookupTable;
	}
}

dsf2flac_int64 DsdDecimator::getLength()
{
	return reader->getLength()/ratio;
}

dsf2flac_float64 DsdDecimator::getPosition()
{
	return (dsf2flac_float64) (reader->getPosition()-tzero)/ratio;
}

dsf2flac_float64 DsdDecimator::getFirstValidSample() {
	return (dsf2flac_float64)nLookupTable / nStep - (dsf2flac_float64)tzero / ratio;
}

dsf2flac_float64 DsdDecimator::getLastValidSample() {
	return (dsf2flac_float64)getLength() - (dsf2flac_float64)tzero / ratio;
}

dsf2flac_uint32 DsdDecimator::getOutputSampleRate()
{
	return outputSampleRate;
}

bool DsdDecimator::isValid()
{
	return valid;
}

std::string DsdDecimator::getErrorMsg() {
	return errorMsg;
}

void DsdDecimator::initLookupTable(const int nCoefs,const dsf2flac_float64* coefs,const int tz)
{
	tzero = tz;
	// calc how big the lookup table is.
	nLookupTable = (nCoefs+7)/8;
	// allocate the table
	lookupTable = new calc_type*[nLookupTable];
	for (dsf2flac_uint32 n=0; n<nLookupTable; n++)
	{
		lookupTable[n] = new calc_type[256];
		for (int m=0; m<256; m++)
			lookupTable[n][m] = 0;
	}
	// loop over each entry in the lookup table
	for (dsf2flac_uint32 t=0; t<nLookupTable; t++) {
		// how many samples from the filter are spanned in this entry
		int k = nCoefs - t*8;
		if (k>8) k=8;
		// loop over all possible 8bit dsd sequences
		for (int dsdSeq=0; dsdSeq<256; ++dsdSeq) {
			dsf2flac_float64 acc = 0.0;
			for (int bit=0; bit<k; bit++) {
				dsf2flac_float64 val;
				if (reader->msbIsPlayedFirst()) {
					val = -1 + 2*(dsf2flac_float64) !!( dsdSeq & (1<<(7-bit)) );
				} else {
					val = -1 + 2*(dsf2flac_float64) !!( dsdSeq & (1<<(bit)) );
				}
				acc += val * coefs[t*8+bit];
			}
			lookupTable[t][dsdSeq] = (calc_type) acc;
		}
	}
	lookupTableAllocated = true;
}

template<> void DsdDecimator::getSamples(dsf2flac_int16 *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude,dsf2flac_float64 clipAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,clipAmplitude,true);
}
template<> void DsdDecimator::getSamples(dsf2flac_int32 *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude,dsf2flac_float64 clipAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,clipAmplitude,true);
}
template<> void DsdDecimator::getSamples(dsf2flac_int64 *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude,dsf2flac_float64 clipAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,clipAmplitude,true);
}
template<> void DsdDecimator::getSamples(dsf2flac_float32 *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude,dsf2flac_float64 clipAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,clipAmplitude,false);
}
template<> void DsdDecimator::getSamples(dsf2flac_float64 *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude,dsf2flac_float64 clipAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,clipAmplitude,false);
}
template <typename sampleType> void DsdDecimator::getSamplesInternal(
		sampleType *buffer,
		dsf2flac_uint32 bufferLen,
		dsf2flac_float64 scale,
		dsf2flac_float64 tpdfDitherPeakAmplitude,
		dsf2flac_float64 clipAmplitude,
		bool roundToInt)
{
	// check the buffer seems sensible
	ldiv_t d = ldiv(bufferLen,getNumChannels());
	if (d.rem) {
		fputs("Buffer length is not a multiple of getNumChannels()",stderr);
		exit(EXIT_FAILURE);
	}
	// flag if we need to clip
	bool clip = clipAmplitude > 0;
	// get the sample buffer
	boost::circular_buffer<dsf2flac_uint8>* buff = reader->getBuffer();
	for (int i=0; i<d.quot ; i++) {
		// filter each chan in turn
		for (dsf2flac_uint32 c=0; c<getNumChannels(); c++) {
			calc_type sum = 0.0;
			for (dsf2flac_uint32 t=0; t<nLookupTable; t++) {
				dsf2flac_uint32 byte = (dsf2flac_uint32) buff[c][t] & 0xFF;
				sum += lookupTable[t][byte];
			}
			sum = sum*scale;
			// dither before rounding/truncating
			if (tpdfDitherPeakAmplitude > 0) {
				// TPDF dither
				calc_type rand1 = ((calc_type) rand()) / ((calc_type) RAND_MAX); // rand value between 0 and 1
				calc_type rand2 = ((calc_type) rand()) / ((calc_type) RAND_MAX); // rand value between 0 and 1
				sum = sum + (rand1-rand2)*tpdfDitherPeakAmplitude;
			}
			if (clip) {
				if (sum > clipAmplitude)
					sum = clipAmplitude;
				else if (sum < -clipAmplitude)
					sum = -clipAmplitude;
			}
			if (roundToInt)
				buffer[i*getNumChannels()+c] = static_cast<sampleType>(round(sum));
			else
				buffer[i*getNumChannels()+c] = static_cast<sampleType>(sum);
		}
		// step the buffer
		for (dsf2flac_uint32 m=0; m<nStep; m++)
			reader->step();
	}
}
