# Simple C/S KV Store

This is a simple single-threaded in-memory KV store which supports `GET`, `PUT`, `DELETE`, `SCAN` operations.

The key length is fixed 8 bytes, and the value is fixed 256 bytes.

## Build

The demo client will be built into `build/client` and the demo server will be built into `build/server`

Build server:

```
make server
```

Build client:

```
make client
```

## Data Structure

This KV store uses B+-tree to store KV pair in leaves.

## Server

The server uses an `EventLoop` and `epoll` as poller to poll networking events and process them. This enables the server to process many connections at the same time.

## Protocol

The protocol is very simple. The first byte in a message indicates the type of the message. The types include:

```
GET, // C->S, GET the corresponding value of KEY
PUT, // C->S, PUT a KV pair
DELETE, // C->S, DELETE a key
SCAN, // C->S, SCAN all kv pairs between two KEYs
KV, // S->C, a KV pair
VALUE, // S->C, simply a VALUE
NO_MORE, // S->C, means there will be no more KV pairs
OK, // S->C, Operation went well
ERROR, // S->C, Something went wrong
```

For different types of message, the length of the data followed varies. 