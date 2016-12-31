#ifndef CNULLSTREAM_H
#define CNULLSTREAM_H

class CNullStream {};

template <typename T> CNullStream operator<<(CNullStream s, const T &) {return s;}

#endif // CNULLSTREAM_H
