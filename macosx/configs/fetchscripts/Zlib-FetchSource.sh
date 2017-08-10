#!/bin/sh

VerLib="1.2.11"
OutDir="zlib"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://zlib.net/${FileName}"
MD5Sum="1c9f62f0778697a09d36121ead88e08e"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
