# foodculator

![CI](https://github.com/lelika1/foodculator/workflows/CI/badge.svg)

## Build

```sh
$ sudo apt install libsqlite3-dev
$ mkdir build && cd build
$ cmake .. && make
```

## Run

```sh
$ foodculator path/to/folder/static /tmp/database.db
```

## Build with Docker

```sh
$ docker build .
```

## Run in Docker


```sh
$ docker run -p 1234:1234 --rm \
   --label=com.centurylinklabs.watchtower.stop-signal=SIGKILL \
   -v /root/foodprod.db:/db.db \
   --name foodculator \
   docker.pkg.github.com/lelika1/foodculator/foodculator:latest
```
