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
  * dsd_decimator.cpp
  * 
  * Implementation of the class dsdDecimator.
  * 
  * The dsdDecimator class does the actual conversion from dsd to pcm. Pass in a dsdSampleReader
  * to the create function along with the desired pcm sample rate (must be multiple of 44.1k).
  * Then you can simply read pcm samples into a int or float buffer using getSamples.
  * 
  */

#include "dsd_decimator.h"
#include <math.h>
#include "filters.cpp"

static bool lookupTableAllocated = false;

/**
 * dsdDecimator::dsdDecimator(dsdSampleReader *r, dsf2flac_uint32 rate)
 * 
 * Constructor!
 * 
 * Pass in a dsdSampleReader and the desired output sample rate.
 * 
 */
dsdDecimator::dsdDecimator(dsdSampleReader *r, dsf2flac_uint32 rate)
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
		initLookupTable(nCoefs_176,coefs_176,tzero_176);
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

/**
 * dsdDecimator::~dsdDecimator
 * 
 * Destructor! Free the lookuptable
 * 
 */
dsdDecimator::~dsdDecimator()
{
	if (lookupTableAllocated) {
		for (dsf2flac_uint32 n=0; n<nLookupTable; n++)
		{
			delete[] lookupTable[n];
		}
		delete[] lookupTable;
	}
}

/**
 * unsigned long long int dsdDecimator::dsdDecimator::getLength()
 * 
 * Return the data length in output samples
 * 
 */
dsf2flac_int64 dsdDecimator::getLength()
{
	return reader->getLength()/ratio;
}
/**
 * unsigned long long int dsdDecimator::dsdDecimator::getPosition()
 * 
 * Return the current position in output samples
 * 
 */
dsf2flac_float64 dsdDecimator::getPosition()
{
	return (dsf2flac_float64)reader->getPosition(tzero)/ratio;
}
/**
 * dsf2flac_float64 dsdDecimator::dsdDecimator::getFirstValidSample()
 * 
 * return the position of the first output sample that is completely defined.
 */
dsf2flac_float64 dsdDecimator::dsdDecimator::getFirstValidSample() {
	return (dsf2flac_float64)nLookupTable / nStep - (dsf2flac_float64)tzero / ratio;
}
/**
 * dsf2flac_float64 dsdDecimator::dsdDecimator::getLastValidSample()
 * 
 * return the position of the last output samples that is compeltely defined.
 */
dsf2flac_float64 dsdDecimator::dsdDecimator::getLastValidSample() {
	return (dsf2flac_float64)getLength() - (dsf2flac_float64)tzero / ratio;
}
/**
 * dsf2flac_uint32 dsdDecimator::getOutputSampleRate()
 * 
 * Return the output sample rate
 * 
 */
dsf2flac_uint32 dsdDecimator::getOutputSampleRate()
{
	return outputSampleRate;
}

/**
 * bool dsdDecimator::isValid()
 * 
 * Return false if the reader is invalid (format/file error for example).
 * 
 */
bool dsdDecimator::isValid()
{
	return valid;
}

/**
 * bool dsdDecimator::getErrorMsg()
 * 
 * Returns a message describing the last error which caused the reader to become invalid.
 * 
 */
std::string dsdDecimator::getErrorMsg() {
	return errorMsg;
}

/**
 * void dsdDecimator::initLookupTable(const dsf2flac_float64 nCoefs,const dsf2flac_float64* coefs)
 * 
 * private method. Initialises the lookup table used for very fast filtering.
 * 
 */
void dsdDecimator::initLookupTable(const int nCoefs,const dsf2flac_float64* coefs,const int tz)
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
				if (reader->msbIsYoungest())
					val = -1 + 2*(dsf2flac_float64) !!( dsdSeq & (1<<(7-bit)) );
				else
					val = -1 + 2*(dsf2flac_float64) !!( dsdSeq & (1<<(bit)) );
				acc += val * coefs[t*8+bit];
			}
			lookupTable[t][dsdSeq] = (calc_type) acc;
		}
	}
	lookupTableAllocated = true;
}


/**
 * template <typename sampleType> void getSamples(sampleType *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude);
 * 
 * Read pcm output samples in format sampleType into a buffer of length bufferLen.
 * bufferLen must be a multiple of getNumChannels(). Channels are interleaved into the buffer.
 * 
 * You also need to provide a scaling factor. This is particularly important for int sample types. The raw dsd data has peak amplitude +-1.
 * 
 * If you wish to add TPDF dither to the data before quantization then please also provide the peak amplitude.
 * 
 * These sample types are supported:
 * 
 * short int
 * int
 * long int
 * float
 * dsf2flac_float64
 * 
 * Others should be very simple to add (just take a look at the templates below).
 * 
 */
template<> void dsdDecimator::getSamples(short int *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,true);
}
template<> void dsdDecimator::getSamples(int *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,true);
}
template<> void dsdDecimator::getSamples(long int *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,true);
}
template<> void dsdDecimator::getSamples(float *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,false);
}
template<> void dsdDecimator::getSamples(double *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude)
{
	getSamplesInternal(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,false);
}

/**
 * template <typename sampleType> void dsdDecimator::getSamplesInternal(sampleType *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude, bool roundToInt)
 *  
 * private method. Does the actual calculation for the getSamples method. Using the lookup tables FIR calculation is a pretty simple summing operation.
 * 
 */

template <typename sampleType> void dsdDecimator::getSamplesInternal(sampleType *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude, bool roundToInt)
{
	// check the buffer seems sensible
	div_t d = div(bufferLen,getNumChannels());
	if (d.rem) {
		fputs("Buffer length is not a multiple of getNumChannels()",stderr);
		exit(EXIT_FAILURE);
	}
	// get the sample buffer
	boost::circular_buffer<dsf2flac_uint8>* buff = (*reader).getBuffer();
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
				calc_type rand1 = (calc_type)(rand()) / (calc_type)(RAND_MAX); // rand value between 0 and 1
				calc_type rand2 = (calc_type)(rand()) / (calc_type)(RAND_MAX); // rand value between 0 and 1
				sum = sum + (rand1-rand2)*tpdfDitherPeakAmplitude;
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
