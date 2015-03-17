FROM debian:wheezy

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update
RUN apt-get install -y \
    lighttpd \
    build-essential \
    automake \
    libtool \
    git \
    libfcgi-dev \
    libjsoncpp-dev \
    liburiparser-dev \
    libneon27-dev \
    libsqlite3-dev

RUN mkdir /build
RUN cd /build && \
    git clone https://github.com/moravianlibrary/libalgo && \
    cd libalgo && \
    libtoolize && \
    autoreconf -i && \
    ./configure && \
    make && \
    make install && \
    ldconfig /usr/local/lib

COPY lighttpd-detectproj.conf /etc/lighttpd/conf-available/30-detectproj.conf
RUN lighttpd-enable-mod fastcgi
RUN lighttpd-enable-mod detectproj

RUN mkdir detectproj
COPY projections.txt /usr/local/detectproj/projections.txt
COPY src/main.cpp /build/detectproj/main.cpp
COPY src/output.cpp /build/detectproj/output.cpp
COPY src/output.h /build/detectproj/output.h
COPY src/http.cpp /build/detectproj/http.cpp
COPY src/http.h /build/detectproj/http.h
COPY src/sqlite.cpp /build/detectproj/sqlite.cpp
COPY src/sqlite.h /build/detectproj/sqlite.h
COPY src/detectproj.cpp /build/detectproj/detectproj.cpp
COPY src/detectproj.h /build/detectproj/detectproj.h
RUN g++ /build/detectproj/*.cpp -ansi -O2 -lfcgi -ljsoncpp -luriparser -lalgo -lneon -lsqlite3 -o /usr/local/bin/detectproj

COPY init.sh /init.sh

ENTRYPOINT ["/init.sh"]
