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
 
 /**
  * dsd_decimator.h
  * 
  * Header file for the class dsdDecimator.
  * 
  * The dsdDecimator class does the actual conversion from dsd to pcm. Pass in a dsdSampleReader
  * to the create function along with the desired pcm sample rate (must be multiple of 44.1k).
  * Then you can simply read pcm samples into a int or float buffer using getSamples.
  * 
  */
  
#define calc_type dsf2flac_float64 // you can change the type used to do the filtering... but there is barely any change in calc speed between float and double

#ifndef DSDDECIMATOR_H
#define DSDDECIMATOR_H

#include "dsd_sample_reader.h"

/**
 *
 * The DsdDecimator reads DSD samples from a DsdSampleReader and converts them to PCM samples.
 *
 * The DsdDecimator only supports output sample rates which are multiples of 44.1kHz.
 */
class DsdDecimator
{
public:
	/**
	 * Class constructor.
	 * DsdSampleReader must be a valid reader.
	 * outputSampleRate sets the sampling frequency for the output PCM samples, must be a multiple of 44100.
	 * Note that not all output sample rates are supported by default.
	 * Most can be easily added by putting an appropriate filter into the filters.cpp file.
	 */
	DsdDecimator(DsdSampleReader *reader, dsf2flac_uint32 outputSampleRate);
	/// Class destructor, frees internal buffers and lookup table.
	virtual ~DsdDecimator();

	/// Return false if the reader is invalid (format/file error for example).
	bool isValid();
	/// Returns a message explaining why the reader is invalid.
	std::string getErrorMsg();

	/// Return the output sample rate in Hz.
	dsf2flac_uint32 getOutputSampleRate();
	/// Return the decimation ratio: DSD sample rate / PCM sample rate.
	dsf2flac_uint32 getDecimationRatio() {return ratio;};
	/// Return the data length in PCM samples.
	dsf2flac_int64 getLength();
	/// Return the number of channels if audio data.
	dsf2flac_uint32 getNumChannels() { return reader->getNumChannels(); };
	/// Return the current position in PCM samples.
	dsf2flac_float64 getPosition();
	/// Return the current position in seconds.
	dsf2flac_float64 getPositionInSeconds() { return getPosition()/outputSampleRate; };
	/// Return the current position as a percent of the total data length.
	dsf2flac_float64 getPositionAsPercent() { return getPosition()/getLength()*100; };
	/// Return the position of the first PCM sample that is completely defined.
	dsf2flac_float64 getFirstValidSample();
	/// Return the position of the last PCM sample that is completely defined.
	dsf2flac_float64 getLastValidSample();
	/// Steps the decimator forward by 8 DSD samples.
	void step() { reader->step(); };
	/**
	 * Read PCM output samples in format sampleType into a buffer of length bufferLen.
	 * bufferLen must be a multiple of getNumChannels().
	 * Channels are interleaved into the buffer.
	 *
	 * You also need to provide a scaling factor. This is particularly important for int sample types. The raw DSD data has peak amplitude +-1.
	 *
	 * If you wish to add TPDF dither to the data before quantization then please also provide the peak amplitude.
	 * You can also choose to clip the data above a certain amplitude, set to <=0 for no clipping but be warned this can cause strange overflow behaviour.
	 *
	 * These sample types are supported:
	 *
	 * int16
	 * int32
	 * int64
	 * float32
	 * float64
	 *
	 * Others should be very simple to add (just take a look at the templates in the source code).
	 *
	 */
	template <typename sampleType> void getSamples(
			sampleType *buffer,
			dsf2flac_uint32 bufferLen,
			dsf2flac_float64 scale,
			dsf2flac_float64 tpdfDitherPeakAmplitude = 0,
			dsf2flac_float64 clipAmplitude = 0);
private:	// private methods
	/// Initializes the filter lookup table.
	void initLookupTable(const dsf2flac_int32 nCoefs,const dsf2flac_float64* coefs,const dsf2flac_int32 tzero);
	/// Does the actual calculation for the getSamples method. Using the lookup tables FIR calculation is a pretty simple summing operation.
	template <typename sampleType> void getSamplesInternal(
			sampleType *buffer,
			dsf2flac_uint32 bufferLen,
			dsf2flac_float64 scale,
			dsf2flac_float64 tpdfDitherPeakAmplitude,
			dsf2flac_float64 clipAmplitude,
			bool roundToInt);
private:
	DsdSampleReader *reader;
	dsf2flac_uint32 outputSampleRate;
	dsf2flac_uint32 nLookupTable;
	dsf2flac_uint32 tzero; // filter t=0 position
	calc_type** lookupTable;
	dsf2flac_uint32 ratio; // inFs/outFs
	dsf2flac_uint32 nStep;
	bool valid;
	std::string errorMsg;
};

#endif // DSDDECIMATOR_H
