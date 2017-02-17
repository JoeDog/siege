FROM frolvlad/alpine-glibc:alpine-3.5

COPY build/siege /usr/bin/siege
COPY build/siege.config /usr/bin/siege.config
COPY build/bombardment /usr/bin/bombardment

ENTRYPOINT ["siege"]

CMD ["--help"]

