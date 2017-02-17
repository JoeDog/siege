FROM frolvlad/alpine-glibc:alpine-3.5

COPY build/siegerc /usr/local/etc/siegerc
COPY build/siege /usr/bin/siege

RUN mkdir -p /usr/local/var/ \
    && touch /usr/local/var/siege.log

ENTRYPOINT ["siege"]

CMD ["--help"]

