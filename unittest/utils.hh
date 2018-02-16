#ifndef _DL_UTILS_
#define _DL_UTILS_

// produces a stack backtrace with demangled function & method names.
std::string stacktrace(int skip = 1);

// load file to output stream
void read_file(const std::string& path, std::ostream& data);

#endif /*_DL_UTILS_*/

