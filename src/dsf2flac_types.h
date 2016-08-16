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

// correct for linux AMD64, other platforms must be added!
typedef char 				dsf2flac_int8;
typedef short int			dsf2flac_int16;
typedef int					dsf2flac_int32;
typedef long int			dsf2flac_int64;

typedef unsigned char 		dsf2flac_uint8;
typedef unsigned short int	dsf2flac_uint16;
typedef unsigned int		dsf2flac_uint32;
typedef unsigned long int	dsf2flac_uint64;

typedef	float				dsf2flac_float32;
typedef double				dsf2flac_float64;
