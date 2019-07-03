#ifndef _____SWAN_CODEC_HPP_____
#define _____SWAN_CODEC_HPP_____
#include "Swan.hpp"
#include<iosfwd>
#include<sstream>
#include<string>
#include<utf8.h>
#include<boost/iostreams/concepts.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/invert.hpp>
#include<boost/iostreams/device/back_inserter.hpp>
#include<boost/iostreams/device/array.hpp>

#define CFE_ENCODE_VALID_STRING 1
#define CFE_DECODE_VALID_STRING 2

namespace Swan {

/** The class representing a codec to convert from/to various encodings */
struct Codec {
virtual std::string decode (const std::string& source);
virtual std::string encode (const std::string& source);
virtual void transcode (boost::iostreams::filtering_istream& in, bool encode) = 0;
virtual void transcode (boost::iostreams::filtering_ostream& in, bool encode) = 0;
virtual int getFlags () = 0;
virtual ~Codec () = default;
};

} // namespace Swan
#endif
