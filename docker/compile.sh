apt-get update \
&& apt-get install -y build-essential automake curl openssl jq zlib1g-dev \
&& LATEST=$(curl -s https://api.github.com/repos/JoeDog/siege/tags | jq  --raw-output '.[0]["tarball_url"]') \
&& curl -L -o siege.tar.gz $LATEST \
&& tar xvvzf siege.tar.gz \
&& cd JoeDog* \
&& utils/bootstrap \
&& ./configure \
&& make \
&& make install \
&& cp src/siege /opt/siege \
&& cp src/bombardment /opt/siege \
&& cp src/siege.config /opt/siege

