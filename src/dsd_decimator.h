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

#include <dsd_sample_reader.h>

class dsdDecimator
{
public:
	dsdDecimator(dsdSampleReader *reader, dsf2flac_uint32 outputSampleRate);
	virtual ~dsdDecimator();
	// Method to get the output sampels
	template <typename sampleType> void getSamples(sampleType *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude = 0);
	dsf2flac_uint32 getOutputSampleRate();
	dsf2flac_int64 getLength(); // return total length in samples of decimated data
	dsf2flac_float64 getPosition();
	dsf2flac_float64 getFirstValidSample();
	dsf2flac_float64 getLastValidSample();
	dsf2flac_float64 getPositionInSeconds() { return getPosition()/outputSampleRate; };
	dsf2flac_float64 getPositionAsPercent() { return getPosition()/getLength()*100; };
	dsf2flac_uint32 getNumChannels() { return reader->getNumChannels(); };
	bool isValid(); // return false if the decimator is invalid
	std::string getErrorMsg(); // returns a human readable error message
	void step() { reader->step(); }; // handy wrappers.
	dsf2flac_uint32 getDecimationRatio() {return ratio;};
private:
	dsdSampleReader *reader;
	dsf2flac_uint32 outputSampleRate;
	dsf2flac_uint32 nLookupTable;
	dsf2flac_uint32 tzero; // filter t=0 position
	calc_type** lookupTable;
	dsf2flac_uint32 ratio; // inFs/outFs
	dsf2flac_uint32 nStep;
	bool valid;
	std::string errorMsg;
	// private methods
	void initLookupTable(const dsf2flac_int32 nCoefs,const dsf2flac_float64* coefs,const dsf2flac_int32 tzero);
	template <typename sampleType> void getSamplesInternal(sampleType *buffer, dsf2flac_uint32 bufferLen, dsf2flac_float64 scale, dsf2flac_float64 tpdfDitherPeakAmplitude, bool roundToInt);
	
};

#endif // DSDDECIMATOR_H
