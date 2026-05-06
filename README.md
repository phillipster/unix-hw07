# ChatFeliPThread
### hw07 - by Felipe Muggiati Feldman

## Usage
This program is split into two components: a server and client.

To compile the entire thing, run `make all`. Ensure that all three .c files,
as well as the header, are in the same directory as the Makefile.

### Server
`usage: server [port number]`

The Server requires no arguments but has an optional argument for port
number; its default value is 13000, as is the default port value for Client.
Here are two valid uses:
```shell
./server
./server 13000
```
This one is straightforward to start up! Note that there are no flags for
positional arguments, since there is only one.

### Client
`usage: client <username> [-p port] [-a address]`

The Client will not start unless a username is provided. Here are a few sample
ways to start the program that have been validated to work:
```shell
./client ClientName
./client ClientName -p 13000
./client ClientName -a localhost
./client ClientName -p 13000 -a localhost
./client ClientName 13000 localhost
```
Note in the last case that if no flags are set, the client will assume that 
the second argument is the port, while the third is the address.

**For testing purposes**, please stick to the port and address seen above
(localhost); you may also enter whatever port the server was given to run on.

## CLI
On the client side, simply type in a message and hit enter to send it to
others. Pressing enter will send an empty line to other users; hitting Ctrl + D
or Ctrl + C will result in disconnection (this program currently lacks
signal handling support, which will be added at a later date).

On the server side, there is no command line interface at all. Notifications
(logging, as described by the spec) will appear every time a new user connects
or a previous user disconnects from the server. Hitting Ctrl + C will terminate
the server and all its users, who are configured to terminate whenever
