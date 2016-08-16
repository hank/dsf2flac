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


#include "tagConversion.h"
#include "id3/misc_support.h"

FLAC__StreamMetadata* id3v2_to_flac(ID3_Tag id3tags) {
	
	
	FLAC__StreamMetadata* flacTags;
	flacTags = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	bool err = false;
	char* tag;
	
	// ARTIST
	tag = latin1_to_utf8 (ID3_GetArtist ( &id3tags ));
	if (tag != NULL)
	{
		bool err1 = !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", tag);
		if (!err1)
			err |= !FLAC__metadata_object_vorbiscomment_append_comment(flacTags, entry, /*copy=*/false); // copy=false: let metadata object take control of entry's allocated string
		err |= err1;
	}
	
	// ALBUM
	tag = latin1_to_utf8 (ID3_GetAlbum ( &id3tags ));
	if (tag != NULL)
	{
		bool err1 = !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", tag);
		if (!err1)
			err |= !FLAC__metadata_object_vorbiscomment_append_comment(flacTags, entry, /*copy=*/false); // copy=false: let metadata object take control of entry's allocated string
		err |= err1;
	}
	
	// TITLE
	tag = latin1_to_utf8 (ID3_GetTitle ( &id3tags ));
	if (tag != NULL)
	{
		bool err1 = !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", tag);
		if (!err1)
			err |= !FLAC__metadata_object_vorbiscomment_append_comment(flacTags, entry, /*copy=*/false); // copy=false: let metadata object take control of entry's allocated string
		err |= err1;
	}
	
	// TRACK
	tag = latin1_to_utf8 (ID3_GetTrack ( &id3tags ));
	if (tag != NULL)
	{
		bool err1 = !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKNUMBER", tag);
		if (!err1)
			err |= !FLAC__metadata_object_vorbiscomment_append_comment(flacTags, entry, /*copy=*/false); // copy=false: let metadata object take control of entry's allocated string
		err |= err1;
	}
	
	// YEAR
	tag = latin1_to_utf8 (ID3_GetYear ( &id3tags ));
	if (tag != NULL)
	{
		bool err1 = !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "DATE", tag);
		if (!err1)
			err |= !FLAC__metadata_object_vorbiscomment_append_comment(flacTags, entry, /*copy=*/false); // copy=false: let metadata object take control of entry's allocated string
		err |= err1;
	}
	
	
	if (err)
		return NULL;
	
	return flacTags;
}

char* latin1_to_utf8(char* latin1) {
	return reinterpret_cast<char*>(latin1_to_utf8( reinterpret_cast<unsigned char*>(latin1)));
}
unsigned char* latin1_to_utf8(unsigned char* latin1)
{
	if (latin1==NULL)
		return NULL;
		
	// count latin1
	int n=0;
	while (latin1[n])
		n++;
	// make buffer for converted chars
	unsigned char* utf8 = new unsigned char[n*2+1];
	unsigned char* utf8_t = utf8;

	while (*latin1) {
		if (*latin1<128)
			*utf8++=*latin1++;
		else
			*utf8++=0xc2+(*latin1>0xbf), *utf8++=(*latin1++&0x3f)+0x80;
	}
	*utf8='\0';
	
	return utf8_t;
}
