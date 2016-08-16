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

#ifndef DOPPACKER_H_
#define DOPPACKER_H_

#include "dsf2flac_types.h"
#include "dsd_sample_reader.h"


/**
 * A class which takes DSD samples from a DsdSampleReader and packs them into DOP encoded PCM.
 *
 */
class DopPacker {
public:
	/**
	 * Class constructor. reader can be any type of DSD sample reader.
	 */
	DopPacker(DsdSampleReader *reader);

	/**
	 * Class destructor.
	 */
	virtual ~DopPacker();

	/**
	 * Read DOP PCM samples from the reader.
	 * The input "buffer" will be filled with DoP encoded 24bit PCM samples.
	 * "buffer" must be at least "bufferLen" long.
	 * "bufferLen" must be a multiple of the number of channels in the reader, a horrible error will be thrown if it is not.
	 * The pcm samples are packed in increasing time and interleaved by channel i.e. [left0 right0 left1 right1 ... leftN rightN]
	 */
	void pack_buffer(dsf2flac_int32 *buffer, dsf2flac_uint32 bufferLen);

private:


	DsdSampleReader *reader;	//!< A pointer to the DsdSampleReader.
};

#endif /* DOPPACKER_H_ */
