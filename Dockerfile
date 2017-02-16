FROM alpine:3.5

ENV SIEGE_VERSION 4.0.3rc4
ENV BUILD_DEPS autoconf \
         automake \
         curl \
         file \
         gcc \
         libc-dev \
         libtool \
         make \
         perl

RUN apk add --no-cache $BUILD_DEPS \
    && apk add --no-cache zlib zlib-dev \
    && curl -fSL https://github.com/jstarcher/siege/archive/v$SIEGE_VERSION.tar.gz -o siege.tar.gz \
    && mkdir -p /usr/src \
    && tar -zxC /usr/src -f siege.tar.gz \
    && rm siege.tar.gz \
    && cd /usr/src/siege-$SIEGE_VERSION \
    && utils/bootstrap \
    && ./configure \
    && make \
    && make install \
    && cd / \
    && rm -rf /usr/src/siege-$SIEGE_VERSION \
    && apk del $BUILD_DEPS

ENTRYPOINT ["siege"]

CMD ["--help"]

