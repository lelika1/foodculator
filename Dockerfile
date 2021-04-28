# Container image that runs your code
FROM alpine:3.12.0 as BASE
RUN apk --no-cache add sqlite-dev libstdc++ zlib-dev openssl-dev curl-dev boost-dev

FROM BASE as builder
RUN apk --no-cache add build-base clang python3 cmake
COPY . /src
RUN mkdir /build \
 && cd /build \
 && CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake /src \
 && make -j$(nproc)
RUN /build/tests/tests

FROM BASE
COPY --from=builder /build/src/foodculator /app/
COPY --from=builder /src/static /app/static
ARG version="UNKNOWN"
ENV VERSION="$version"
ENTRYPOINT ["/app/foodculator", "/app/static", "/data/db.db"]
