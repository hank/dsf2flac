
#include "FLAC++/metadata.h"
#include "id3/tag.h"

FLAC__StreamMetadata* id3v2_to_flac(ID3_Tag id3);

char* latin1_to_utf8(char* latin1);
unsigned char* latin1_to_utf8(unsigned char* latin1);