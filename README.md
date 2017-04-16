# async-study

The goal is to understand the different between synchronous and asynchronous communication.
In src/ there are three programs that do the same thing but using different communication method.

All details on http://julien.gunnm.org/programming/linux/2017/04/15/comparison-sync-vs-async/

## What is there?
All these programs make N requests to a webserver and prints the reply. The difference
is how they process the request:
 * src/epoll.c:  use asynchronous communication ith the Linux epoll framework.
 * src/sync.c: spawn a thread for each communication and use synchronous communication.
 * src/posix/async.c: is an attempt to use the POSIX async API. Since the API is crap, I stop trying.

## How to build the programs?
```
make clean all
```

Note: async does not work. This is just an attempt to use the POSIX async-io framework

## How to use it?

```
./[sync|epoll] hostname 80
```

It will then issue a request to <hostname> on port 80 and print the result.

If you want to make some performance measurement, do:

```
time ./[sync|epoll] hostname 80
```

## How to modify the number of requests being issued?

Modify the NREQUEST variable in the Makefile.

Then, rebuild everything:

```
make clean all
```


## More information
You can read the related blog post: http://julien.gunnm.org/programming/linux/2017/04/15/comparison-sync-vs-async/
