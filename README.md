# foodculator

![CI](https://github.com/lelika1/foodculator/workflows/CI/badge.svg)

## Build

```sh
$ sudo apt-get install g++ make binutils cmake libssl-dev libboost-system-dev zlib1g-dev libsqlite3-dev
$ mkdir build && cd build
$ cmake .. && make -j4
```

## Run

```sh
$ foodculator path/to/folder/static /tmp/database.db
```

* `/version` http handler exposes the value of `VERSION` env variable.
* `PORT` env variable is used to override the port (`1234` by default).

## Build with Docker

```sh
$ docker build .
```

## Run in Docker


```sh
$ docker run -p 1234:1234 --rm -d \
   --label=com.centurylinklabs.watchtower.stop-signal=SIGKILL \
   -v /root/foodprod:/data \
   --name foodculator \
   docker.pkg.github.com/lelika1/foodculator/foodculator:latest
```
