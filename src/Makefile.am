SUBDIRS = libdstdec
AM_CPPFLAGS= $(LIBFLACPP_CFLAGS) $(ID3_CPPFLAGS) $(BOOST_CPPFLAGS) -O3 -Wall
AM_LDFLAGS= $(ID3_LDFLAGS) $(BOOST_LDFLAGS) $(BOOST_CHRONO_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) $(BOOST_TIMER_LIB)

bin_PROGRAMS=dsf2flac
dsf2flac_SOURCES=cmdline.cpp dsd_decimator.cpp dsdiff_file_reader.cpp dsd_sample_reader.cpp dsf_file_reader.cpp filters.cpp fstream_plus.cpp main.cpp tagConversion.cpp dop_packer.cpp
dsf2flac_LDADD= $(LIBFLACPP_LIBS) $(LIBFLACPP_LIBDIR) $(ID3_LIBS) libdstdec/libdstdec.a
