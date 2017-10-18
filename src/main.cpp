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

#include <boost/timer/timer.hpp>
#include <boost/filesystem.hpp>
#include <FLAC++/metadata.h>
#include <FLAC++/encoder.h>
#include <sstream>
#include <math.h>
#include "cmdline.h"
#include "dsd_decimator.h"
#include "dsf_file_reader.h"
#include "dsdiff_file_reader.h"
#include "tagConversion.h"
#include "dop_packer.h"

#define flacBlockLen 1024

using boost::timer::cpu_timer;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;

static nanosecond_type reportInterval(100000000LL);
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
 * called to report progress to the user.
 */
void checkTimer(dsf2flac_float64 currPos, dsf2flac_float64 percent)
{
	cpu_times const elapsed_times(timer.elapsed());
	nanosecond_type const elapsed(elapsed_times.system + elapsed_times.user);
	if (elapsed >= reportInterval) {
		fprintf(stderr,"\33[2K\r");
		fprintf(stderr,"Rate: %4.1fx\t",(currPos-lastPos)/elapsed*1000000000);
		fprintf(stderr,"Progress: %3.0f%%",percent);
		fflush(stderr);
		lastPos = currPos;
		timer = cpu_timer();
	}
}

/**
 * muti_track_name_helper
 *
 * little helper to construct file names for multiple track files.
 */
boost::filesystem::path muti_track_name_helper(boost::filesystem::path outpath, int n) {
	std::ostringstream prefix;
	prefix << "track ";
	prefix << n+1;
	prefix << " - ";
	boost::filesystem::path trackOutPath = outpath.parent_path() / (prefix.str() + outpath.filename().string());
	return trackOutPath;
}

/**
 * int track_helper()
 * 
 * converts a track at a time to PCM FLAC
 * 
 */
bool pcm_track_helper(
	boost::filesystem::path outpath,
	DsdDecimator* dec,
	int bits,
	dsf2flac_float64 scale,
	dsf2flac_float64 tpdfDitherPeakAmplitude,
	dsf2flac_float64 clipAmplitude,
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
	FLAC__StreamMetadata* metadata[2];

	// setup the encoder
	if(!encoder) {
		fprintf(stderr, "ERROR: allocating encoder\n");
		return false;
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

	// return if there is a problem with any of the flac stuff.
	if (!ok)
		return ok;

	// creep up to the start point.
	while (dec->getPosition() < startPos) {
		dec->step();
	}
	// create a FLAC__int32 buffer to hold the samples as they are converted
	unsigned int bufferLen = dec->getNumChannels()*flacBlockLen;
	FLAC__int32* buffer = new FLAC__int32[bufferLen];
	// MAIN CONVERSION LOOP //
	while (dec->getPosition() <= endPos-flacBlockLen) {
		dec->getSamples(buffer,bufferLen,scale,tpdfDitherPeakAmplitude,clipAmplitude);
		if(!(ok = encoder.process_interleaved(buffer, flacBlockLen)))
			fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
		checkTimer(dec->getPositionInSeconds(),dec->getPositionAsPercent());
	}
	// delete the old buffer and make a new one with a single sample per chan
	delete[] buffer;
	buffer = new FLAC__int32[dec->getNumChannels()];
	// creep up to the end
	while (dec->getPosition() <= endPos) {
		dec->getSamples(buffer,dec->getNumChannels(),scale,tpdfDitherPeakAmplitude,clipAmplitude);
		if(!(ok = encoder.process_interleaved(buffer, 1)))
			fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
		checkTimer(dec->getPositionInSeconds(),dec->getPositionAsPercent());
	}
	delete[] buffer;
	// close the flac file
	ok &= encoder.finish();
	// report back to the user
	fprintf(stderr,"\33[2K\r");
	fprintf(stderr,"%3.1f%%\t",dec->getPositionAsPercent());
	if (ok) {
		fprintf(stderr,"Conversion completed sucessfully.\n");
	} else {
		fprintf(stderr,"\nError during conversion.\n");
		fprintf(stderr, "encoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
	}
	// free things
	FLAC__metadata_object_delete(metadata[0]);
	FLAC__metadata_object_delete(metadata[1]);

	return ok;
}

/*
 * do_pcm_conversion
 *
 * this function uses a dsdDecimator to do the conversion into PCM.
 */
bool do_pcm_conversion(
		DsdSampleReader* dsr,
		int fs,
		int bits,
		bool dither,
		dsf2flac_float64 userScale,
		boost::filesystem::path inpath,
		boost::filesystem::path outpath,
		bool onefile
		)
{

	bool ok = true;

	// create decimator
	DsdDecimator dec(dsr,fs);
	if (!dec.isValid()) {
		fprintf(stderr,"%s\n",dec.getErrorMsg().c_str());
		return false;
	}

	// calc real scale and dither amplitude
	dsf2flac_float64 scale = userScale * pow(2.0,bits-1); // increase scale by factor of 2^23 (24bit).
	dsf2flac_float64 tpdfDitherPeakAmplitude;
	if (dither)
		tpdfDitherPeakAmplitude = 1.0;
	else
		tpdfDitherPeakAmplitude = 0.0;
	dsf2flac_float64 clipAmplitude = pow(2.0,bits-1)-1; // clip at max range.

	setupTimer(dsr->getPositionInSeconds());

	if(onefile) {
		// convert whole file
		dsf2flac_float64 trackStart = dec.getFirstValidSample();
		dsf2flac_float64 trackEnd = dec.getLastValidSample();

		printf("Output file\n\t%s\n",outpath.c_str());
		// use the pcm_track_helper
		ok &= pcm_track_helper(outpath,&dec,bits,scale,tpdfDitherPeakAmplitude,clipAmplitude,trackStart,trackEnd,NULL);
	} else {
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
			if (dsr->getNumTracks() > 1) {
				trackOutPath = muti_track_name_helper(outpath,n);
			} else {
				trackOutPath = outpath;
			}

			printf("Output file\n\t%s\n",trackOutPath.c_str());
			// use the pcm_track_helper
			ok &= pcm_track_helper(trackOutPath,&dec,bits,scale,tpdfDitherPeakAmplitude,clipAmplitude,trackStart,trackEnd,dsr->getID3Tag(n));

		}
	}

	return ok;
}

/**
 *	dop_track_helper
 */
bool dop_track_helper(
	boost::filesystem::path outpath,
	DsdSampleReader* dsr,
	dsf2flac_int64 startPos,
	dsf2flac_int64 endPos,
	ID3_Tag	id3tag)
{

	// double check the start and end positions!
	if ( startPos > dsr->getLength()-1 )
		startPos = dsr->getLength()-1;
	if ( endPos > dsr->getLength() )
		endPos = dsr->getLength();

	// create a dop packer object.
	DopPacker dopp = DopPacker(dsr);

	// flac vars
	bool ok = true;
	FLAC::Encoder::File encoder;
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamMetadata *metadata[2];

	// setup the encoder
	if(!encoder) {
		fprintf(stderr, "ERROR: allocating encoder\n");
		return false;
	}
	ok &= encoder.set_verify(true);
	ok &= encoder.set_compression_level(5);
	ok &= encoder.set_channels(dsr->getNumChannels());
	ok &= encoder.set_bits_per_sample(24);

	if ( dsr->getSamplingFreq() == 2822400 ) {
		ok &= encoder.set_sample_rate(176400);
	} else if ( dsr->getSamplingFreq() == 5644800 ) {
		ok &= encoder.set_sample_rate(352800);
	} else {
		fprintf(stderr, "ERROR: sample rate not supported by DoP\n");
		return false;
	}
	ok &= encoder.set_total_samples_estimate((endPos - startPos)/16);

	// add tags and a padding block
	if(ok) {
		metadata[0] = id3v2_to_flac( id3tag );
		metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
		metadata[1]->length = 2048; /* set the padding length */
		ok = encoder.set_metadata(metadata, 2);
	}

	// initialize encoder
	if(ok) {
		if (!strcmp(outpath.c_str(),"-"))
			init_status = encoder.init((FILE *)stdout); 
		else // outpath.c_str());
			init_status = encoder.init(outpath.c_str());
		if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			fprintf(stderr, "ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;
		}
	}

	// return if there is a problem with any of the flac stuff.
	if (!ok)
		return ok;

	// creep up to the start point.
	while (dsr->getPosition() < startPos) {
		dsr->step();
	}
	// create a FLAC__int32 buffer to hold the samples as they are converted
	unsigned int bufferLen = dsr->getNumChannels()*flacBlockLen;
	FLAC__int32* buffer = new FLAC__int32[bufferLen];
	// MAIN CONVERSION LOOP //
	while (dsr->getPosition() <= endPos-flacBlockLen*16) {
		dopp.pack_buffer(buffer,bufferLen);
		if(!(ok = encoder.process_interleaved(buffer, flacBlockLen)))
			fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
		checkTimer(dsr->getPositionInSeconds(),dsr->getPositionAsPercent());
	}
	// delete the old buffer and make a new one with a single sample per chan
	delete[] buffer;
	buffer = new FLAC__int32[dsr->getNumChannels()];
	// creep up to the end
	while (dsr->getPosition() <= endPos) {
		dopp.pack_buffer(buffer,dsr->getNumChannels());
		if(!(ok = encoder.process_interleaved(buffer, 1)))
			fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
		checkTimer(dsr->getPositionInSeconds(),dsr->getPositionAsPercent());
	}
	delete[] buffer;
	// close the flac file
	ok &= encoder.finish();
	// report back to the user
	fprintf(stderr,"\33[2K\r");
	fprintf(stderr,"%3.1f%%\t",dsr->getPositionAsPercent());
	if (ok) {
		fprintf(stderr,"Conversion completed sucessfully.\n");
	} else {
		fprintf(stderr,"\nError during conversion.\n");
		fprintf(stderr, "encoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf(stderr, "   state: %s\n", encoder.get_state().resolved_as_cstring(encoder));
	}
	// free things
	FLAC__metadata_object_delete(metadata[0]);
	FLAC__metadata_object_delete(metadata[1]);

	return ok;
}
/**
 * int do_dop_conversion
 *
 * this function handles conversion into DoP encoded flac
 *
 */
bool do_dop_conversion(
		DsdSampleReader* dsr,
		boost::filesystem::path inpath,
		boost::filesystem::path outpath
		)
{
	bool ok = true;

	setupTimer(dsr->getPositionInSeconds());

	// convert each track in the file in turn
	for (dsf2flac_uint32 n = 0; n < dsr->getNumTracks();n++) {

		// get and check the start and end samples
		dsf2flac_uint64 trackStart = dsr->getTrackStart(n);
		dsf2flac_uint64 trackEnd = dsr->getTrackEnd(n);

		// construct an appropriate filename for multi track files.
		boost::filesystem::path trackOutPath;
		if (dsr->getNumTracks() > 1) {
			trackOutPath = muti_track_name_helper(outpath,n);
		} else {
			trackOutPath = outpath;
		}

		fprintf(stderr,"Output file\n\t%s\n",trackOutPath.c_str());
		// use the pcm_track_helper
		ok &= dop_track_helper(trackOutPath,dsr,trackStart,trackEnd,dsr->getID3Tag(n));
	}

	return ok;
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
	bool onefile = args_info.onefile_flag;
	bool dop = args_info.dop_flag;
	dsf2flac_float64 userScaleDB = (dsf2flac_float64) args_info.scale_arg;
	dsf2flac_float64 userScale = pow(10.0,userScaleDB/20);
	boost::filesystem::path inpath(args_info.infile_arg);
	boost::filesystem::path outpath;
	if (args_info.outfile_given)
		outpath = args_info.outfile_arg;
	else {
		outpath = inpath;
		outpath.replace_extension(".flac");
	}

	fprintf(stderr,"%s ",CMDLINE_PARSER_PACKAGE_NAME);
	fprintf(stderr,"%s\n\n",CMDLINE_PARSER_VERSION);

	// pointer to the dsdSampleReader (could be any valid type).
	DsdSampleReader* dsr;

	// create either a reader for dsf or dsd
	if (inpath.extension() == ".dsf" || inpath.extension() == ".DSF")
		dsr = new DsfFileReader((char*)inpath.c_str());
	else if (inpath.extension() == ".dff" || inpath.extension() == ".DFF")
		dsr = new DsdiffFileReader((char*)inpath.c_str());
	else {
		fprintf(stderr,"Sorry, only .dsf or .dff input files are supported\n");
		return 0;
	}

	// check reader is valid.
	if (!dsr->isValid()) {
		fprintf(stderr,"Error opening DSDFF file!\n");
		fprintf(stderr,"%s\n",dsr->getErrorMsg().c_str());
		return 0;
	}
	
	// do the conversion into PCM or DoP
	bool ok = false;
	if (!dop) {
		// feedback some info to the user
		fprintf(stderr,"Input file\n\t%s\n",inpath.c_str());
		fprintf(stderr,"Output format\n\tSampleRate: %dHz\n\tDepth: %dbit\n\tDither: %s\n\tScale: %1.1fdB\n",fs, bits, (dither)?"true":"false",userScaleDB);
		//printf("\tIdleSample: 0x%02x\n",dsr->getIdleSample());
    
		ok = do_pcm_conversion(dsr,fs,bits,dither,userScale,inpath,outpath,onefile);
	} else {
		// feedback some info to the user
		fprintf(stderr,"Input file\n\t%s\n",inpath.c_str());
		fprintf(stderr,"Output format\n\tDSD samples packed as DoP\n");

		ok = do_dop_conversion(dsr,inpath,outpath);
	}
	return ok? 0 : 1;
}
