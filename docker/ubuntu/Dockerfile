FROM ubuntu

RUN apt-get -u update && apt-get -y install build-essential automake libpng12-dev libsdl2-dev libopenal-dev libphysfs-dev libvorbis-dev libtheora-dev libglc-dev libglew1.5-dev libxrandr-dev zip unzip libqt5opengl5-dev libssl-dev qtscript5-dev qt5-default libharfbuzz-dev git cmake

# For now, install SDL >= 2.0.5 from source
ADD https://www.libsdl.org/release/SDL2-2.0.7.tar.gz /tmp/
RUN tar -C /tmp/SDL2 -xvzf /tmp/SDL2-2.0.7.tar.gz \
 && cd /tmp/SDL2 && ./configure && make && sudo make install && cd /tmp \
 && rm /tmp/SDL2-2.0.7.tar.gz \
 && rm /tmp/SDL2

WORKDIR /code
CMD ["sh"]

