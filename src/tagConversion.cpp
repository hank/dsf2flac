
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