#!/bin/sh

VerLib="1.5.28"
OutDir="libpng"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://downloads.sourceforge.net/project/libpng/libpng15/${VerLib}/${FileName}"
MD5Sum="d0647c75aa4ca62707c91e8445e6886a"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
