# Foodculator

![CI](https://github.com/lelika1/foodculator/workflows/CI/badge.svg)

<img src="https://raw.githubusercontent.com/lelika1/foodculator/master/static/icon.svg" height=50 width=50>

`Foodculator` is a web service, written in C++, that allows you to record all your favourite food recipes, catolog your tableware, and, most importantly, calculate nutrition of a dish you are cooking today.

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

## Implementation details

`Foodculator` builds with CMake. All its external dependencies are defined as git submodules for this repo.

We use:

* [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib) - to start an HTTP server from within a C++ process.
* [dropbox/json11](https://github.com/dropbox/json11) - for working with JSON in C++
* [fmtlib/fmt](https://github.com/fmtlib/fmt) - for better & faster string formatting.
* [google/googletest](https://github.com/google/googletest) - for writing unit tests
* `libsqlite3` - it should be installed on your system such that we can statically link with it. 

NOTE: It is recommended to put `foodculator` behind some reverse proxy like `nginx` that would terminate https and do authentication, if needed.

---

Icon is made by <a href="https://www.flaticon.com/authors/wanicon" title="wanicon">wanicon</a> from
            <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a>