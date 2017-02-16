FROM alpine:3.5

ENV SIEGE_VERSION v4.0.3rc3

RUN apk add --no-cache --virtual .build-dep \
         autoconf \
         automake \
         curl \
         file \
         gcc \
         libc-dev \
         libtool \
         make \
         perl \
    && curl -fSL https://github.com/JoeDog/siege/archive/$SIEGE_VERSION.tar.gz -o siege.tar.gz \
    && mkdir -p /usr/src \
    && tar -zxC /usr/src -f siege.tar.gz \
    && rm siege.tar.gz \
    && cd /usr/src/siege-${SIEGE_VERSION:1} \
    && utils/bootstrap \
    && ./configure \
    && make \
    && make install \
    && rm -rf /usr/src/siege-$SIEGE_VERSION \
    && apk del .build-deps

ENTRYPOINT ["siege"]
CMD ["--help"]

