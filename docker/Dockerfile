FROM frolvlad/alpine-glibc:alpine-3.5

COPY siegerc-3.1.4 /usr/local/etc/siegerc
COPY siege-3.1.4 /usr/bin/siege

RUN mkdir -p /usr/local/var/ \
    && touch /usr/local/var/siege.log

ENTRYPOINT ["siege"]

CMD ["--help"]

