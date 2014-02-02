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

#include <boost/timer/timer.hpp>
#include <boost/filesystem.hpp>
#include <dsd_decimator.h>
#include <dsf_file_reader.h>
#include <dsdiff_file_reader.h>
#include <tagConversion.h>
#include "FLAC++/metadata.h"
#include "FLAC++/encoder.h"
#include "math.h"
#include "cmdline.h"
#include <sstream>

#define flacBlockLen 1000

using boost::timer::cpu_timer;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;

static nanosecond_type reportInterval(500000000LL);
static cpu_timer timer;
static dsf2flac_float64 lastPos;

/**
 * void setupTimer(dsf2flac_float64 currPos)
 *
 * called by main to setup the boost timer for reporting progress to the user
 */
void setupTimer(dsf2flac_float64 currPos)
{
	timer = cpu_timer();
	lastPos = currPos;
}

/**
 * void checkTimer(dsf2flac_float64 currPos, dsf2flac_float64 percent)
 *
 * called by main to report progress to the user.
 */
void checkTimer(dsf2flac_float64 currPos, dsf2flac_float64 percent)
{
	cpu_times const elapsed_times(timer.elapsed());
	nanosecond_type const elapsed(elapsed_times.system + elapsed_times.user);
	if (elapsed >= reportInterval) {
		printf("\33[2K\r");
		printf("Rate: %4.2fx\t",(currPos-lastPos)/elapsed*1000000000);
		printf("Progress: %3.1f%%",percent);
		fflush(stdout);
		lastPos = currPos;
		timer = cpu_timer();
	}
}

/**
 * int helper()
 * 
 * converts a track at a time.
 * 
 */
int helper(
	boost::filesystem::path outpath,
	dsdDecimator* dec,
	int bits,
	dsf2flac_float64 scale,
	dsf2flac_float64 tpdfDitherPeakAmplitude,
	dsf2flac_float64 startPos,
	dsf2flac_float64 endPos,
	ID3_Tag	id3tag)
{
	
	if ( startPos > dec->getLength()-1 )
		startPos = dec->getLength()-1;
		
	if ( endPos > dec->getLength() )
		endPos = dec->getLength();
	
	// flac vars
	bool ok = true;
	FLAC::Encoder::File encoder;
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamMetadata *metadata[2];

	// setup the encoder
	if(!encoder) {
		fprintf(stderr, "ERROR: allocating encoder\n");
		return 1;
	}
	ok &= encoder.set_verify(true);
	ok &= encoder.set_compression_level(5);
	ok &= encoder.set_channels(dec->getNumChannels());
	ok &= encoder.set_bits_per_sample(bits);
	ok &= encoder.set_sample_rate(dec->getOutputSampleRate());
	ok &= encoder.set_total_samples_estimate(endPos - startPos);

	// add tags and a padding block
	if(ok) {
		metadata[0] = id3v2_to_flac( id3tag );
		metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
		metadata[1]->length = 2048; /* set the padding length */
		ok = encoder.set_metadata(metadata, 2);
	}
	
	// initialize encoder
	if(ok) {
		init_status = encoder.init(outpath.c_str());
		if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			fprintf(stderr, "ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;
		}
	}
	// creep up to the start point.
	while (dec->getPosition() < startPos) {
		dec->step();
	}
	// create a FLAC__int32 buffer to hold the samples as they are converted
	unsigned int bufferLen = dec->getNumChannels()*flacBlockLen;
	FLAC__int32* buffer = new FLAC__int32[bufferLen];
	// MAIN CONVERSION LOOP //
	while (dec->getPosition() <= endPos-flacBlockLen) {
		dec->getSamples(buffer,bufferLen,scale,tpdfDitherPeakAmplitude);
		if(!(ok = encoder.process_interleaved(buffer, flacBlockLen)))
			fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
		checkTimer(dec->getPositionInSeconds(),dec->getPositionAsPercent());
	}
	// delete the old buffer and make a new one with a single sample per chan
	delete[] buffer;
	buffer = new FLAC__int32[dec->getNumChannels()];
	// creep up to the end
	while (dec->getPosition() <= endPos) {
		dec->getSamples(buffer,dec->getNumChannels(),scale,tpdfDitherPeakAmplitude);
		if(!(ok = encoder.process_interleaved(buffer, 1)))
			fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
		checkTimer(dec->getPositionInSeconds(),dec->getPositionAsPercent());
	}
	delete[] buffer;
	// close the flac file
	ok &= encoder.finish();
	// report back to the user
	printf("\33[2K\r");
	printf("%3.1f%%\t",dec->getPositionAsPercent());
	if (ok) {
		printf("Conversion completed sucessfully.\n");
	} else {
		printf("\nError during conversion.\n");
		fprintf(stderr, "encoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
	}
	// free things
	FLAC__metadata_object_delete(metadata[0]);
	FLAC__metadata_object_delete(metadata[1]);
	return 0;
}


/**
 * int main(int argc, char **argv)
 *
 * Main
 */
int main(int argc, char **argv)
{
	// use the cmdline processor
	gengetopt_args_info args_info;
	if (cmdline_parser (argc, argv, &args_info) != 0)
		exit(1) ;

	// if help or version given then exit now.
	if (args_info.help_given || args_info.version_given)
		exit(1);

	// collect the options
	int fs = args_info.samplerate_arg;
	int bits = args_info.bits_arg;
	bool dither = !args_info.nodither_flag;
	dsf2flac_float64 userScaleDB = (dsf2flac_float64) args_info.scale_arg;
	dsf2flac_float64 userScale = pow(10.0d,userScaleDB/20);
	boost::filesystem::path inpath(args_info.infile_arg);
	boost::filesystem::path outpath;
	if (args_info.outfile_given)
		outpath = args_info.outfile_arg;
	else {
		outpath = inpath;
		outpath.replace_extension(".flac");
	}
	// calc real scale and dither amplitude
	dsf2flac_float64 scale = userScale * pow(2.0d,bits-1); // increase scale by factor of 2^23 (24bit).
	dsf2flac_float64 tpdfDitherPeakAmplitude;
	if (dither)
		tpdfDitherPeakAmplitude = 1;
	else
		tpdfDitherPeakAmplitude = 0;
		
	//
	printf("%s ",CMDLINE_PARSER_PACKAGE_NAME);
	printf("%s\n\n",CMDLINE_PARSER_VERSION);

	// pointer to the dsdSampleReader (could be any valid type).
	dsdSampleReader* dsr;

	// create either a reder for dsf or dsd
	if (inpath.extension() == ".dsf" || inpath.extension() == ".DSF")
		dsr = new dsfFileReader((char*)inpath.c_str());
	else if (inpath.extension() == ".dff" || inpath.extension() == ".DFF")
		dsr = new dsdiffFileReader((char*)inpath.c_str());
	else {
		printf("Sorry, only .dff or .dff input files are supported\n");
		return 0;
	}

	// check reader is valid.
	if (!dsr->isValid()) {
		printf("Error opening DSDFF file!\n");
		printf("%s\n",dsr->getErrorMsg().c_str());
		return 0;
	}
	
	// create decimator
	dsdDecimator dec(dsr,fs);
	if (!dec.isValid()) {
		printf("%s\n",dec.getErrorMsg().c_str());
		return 0;
	}

	// feedback some info to the user
	printf("Input file\n\t%s\n",inpath.c_str());
	printf("Output format\n\tSampleRate: %dHz\n\tDepth: %dbit\n\tDither: %s\n\tScale: %1.1fdB\n",fs, bits, (dither)?"true":"false",userScaleDB);
	//printf("\tIdleSample: 0x%02x\n",dsr->getIdleSample());
	
	setupTimer(dsr->getPositionInSeconds());
	
	// convert each track in the file in turn
	for (dsf2flac_uint32 n = 0; n < dsr->getNumTracks();n++) {
		
		// get and check the start and end samples
		dsf2flac_float64 trackStart = (dsf2flac_float64)dsr->getTrackStart(n) / dec.getDecimationRatio();
		dsf2flac_float64 trackEnd = (dsf2flac_float64)dsr->getTrackEnd(n) / dec.getDecimationRatio();
		if (trackStart < dec.getFirstValidSample())
			trackStart = dec.getFirstValidSample();
		if (trackStart >= dec.getLastValidSample() )
			trackStart = dec.getLastValidSample() - 1;
		if (trackEnd <= dec.getFirstValidSample())
			trackEnd = dec.getFirstValidSample() + 1;
		if (trackEnd > dec.getLastValidSample())
			trackEnd = dec.getLastValidSample();
			
		// construct an appropriate filename for multi track files.
		boost::filesystem::path trackOutPath;
		if (dsr->getNumTracks() > 1)
		{
			std::ostringstream prefix;
			prefix << "track ";
			prefix << n+1;
			prefix << " - ";
			trackOutPath = outpath.parent_path() / (prefix.str() + outpath.filename().string());
		} else
		{
			trackOutPath = outpath;
		}
		
		printf("Output file\n\t%s\n",trackOutPath.c_str());
		
		// use the helper
		helper(trackOutPath,&dec,bits,scale,tpdfDitherPeakAmplitude,trackStart,trackEnd,dsr->getID3Tag(n));
	}
}
