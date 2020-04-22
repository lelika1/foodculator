# Container image that runs your code
FROM alpine:latest as BASE
RUN apk --no-cache add sqlite

FROM BASE as builder
RUN apk --no-cache add build-base clang python cmake sqlite-dev
COPY . /src
RUN mkdir /build && cd /build && cmake /src && make
 
FROM BASE
COPY --from=builder /build/foodculator /src/static/ ./app/
ENTRYPOINT ["app/foodculator", "app/static", "db.db"]
