# Container image that runs your code
FROM alpine:latest as BASE
RUN apk --no-cache add sqlite-dev libstdc++

FROM BASE as builder
RUN apk --no-cache add build-base clang python cmake
COPY . /src
RUN mkdir /build && cd /build && CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake /src && make -j $(( $(nproc) + 1 ))

FROM BASE
COPY --from=builder /build/foodculator /app/
COPY --from=builder /src/static /app/static
ARG version="UNKNOWN"
ENV VERSION="$version"
ENTRYPOINT ["/app/foodculator", "/app/static", "/data/db.db"]
