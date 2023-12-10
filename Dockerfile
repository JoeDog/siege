
ARG OS_VER=20.04
ARG OS_IMAGE=ubuntu

# build stage
FROM ${OS_IMAGE}:${OS_VER} AS builder

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update \
    && apt-get install -y \
    git \
    build-essential \
    autoconf automake autotools-dev \
    zlib1g-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists

ARG SIEGE_VER=v4.1.5
ARG SIEGE_REPO=https://github.com/JoeDog/siege.git

WORKDIR /tmp
RUN git clone $SIEGE_REPO siege \
    && cd siege \
    && git checkout $SIEGE_VER \
    && utils/bootstrap \
    && ./configure \
    && make \
    && make install

# run stage
FROM ${OS_IMAGE}:${OS_VER}
COPY --from=builder /usr/local/ /usr/local/
RUN echo "/usr/local/lib64" >> /etc/ld.so.conf.d/all-libs.conf && ldconfig

ENTRYPOINT ["siege"]
