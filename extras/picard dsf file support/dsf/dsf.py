# DSF stream header information support for Mutagen.
# Copyright 2006 Joe Wreschnig
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.

"""DSF audio stream information and tags."""

import os
import struct

from picard.plugins.dsf.id3_dsf import ID3FileType, delete
from mutagen._util import cdata

__all__ = ["DSF", "Open", "delete", "DSF"]

class error(RuntimeError): pass
class HeaderNotFoundError(error, IOError): pass
class InvalidDSFHeader(error, IOError): pass

class DSFInfo(object):
    """DSF stream information.

    Attributes:
    length - audio length, in seconds
    sample_rate - audio sample rate, in Hz
    """

    def __init__(self, fileobj, offset=None):
        # Warning, offset ignored!
        fileobj.seek(0)
        dsd_header = fileobj.read(28)
        if len(dsd_header) != 28 or not dsd_header.startswith("DSD "):
            raise DSFHeaderError("DSF dsd header not found")
        self.file_size = cdata.ulonglong_le(dsd_header[12:20])
        self.id3_location = cdata.ulonglong_le(dsd_header[20:28])
        fmt_header = fileobj.read(52)
        if len(fmt_header) != 52 or not fmt_header.startswith("fmt "):
            raise DSFHeaderError("DSF fmt header not found")
        self.format_version = cdata.uint_le(fmt_header[12:16])
        self.format_id = cdata.uint_le(fmt_header[16:20])
        self.channel_type = cdata.uint_le(fmt_header[20:24])
        self.channel_num = cdata.uint_le(fmt_header[24:28])
        self.sample_rate = cdata.uint_le(fmt_header[28:32])
        self.bits_per_sample = cdata.uint_le(fmt_header[32:36])
        samples = cdata.ulonglong_le(fmt_header[36:44])
        self.length = float(samples) / self.sample_rate

    def pprint(self):
        return "DSF, %.2f seconds, %d Hz." % (
            self.length, self.sample_rate)

class DSF(ID3FileType):
    """A DSF audio file."""

    _Info = DSFInfo
    _mimes = ["audio/dsf"]

    def score(filename, fileobj, header):
        filename = filename.lower()
        return (header.startswith("DSD ") * 2 + filename.endswith(".dsf"))
    score = staticmethod(score)

Open = DSF

#class EasyMP3(MP3):
#    """Like MP3, but uses EasyID3 for tags."""
#    from mutagen.easyid3 import EasyID3 as ID3

