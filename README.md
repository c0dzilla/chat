# chat
**Chat server in c**

1. Run script
2. nc 3490
3. Admins will be asked for an alias name on entering the secret keyword.
4. Admin can ban a user by prepending name with "ban".


## Theory

This chat server is an application of socket programming.


### Sockets

There are mainly two types of sockets:
* __Stream sockets__: Referred to as SOCK_STREAM, they work on TCP(Transmission Control Protocol) with persistent connection between sockets through which messages are transferred. Messages are received in the order in which they are sent.
* __Datagram sockets__: Referred to as SOCK_DGRAM, they work on UDP(User Datagram Protocol). Data packets with the destination information are sent out. They may or may not reach their destination. They may or may not arrive in same order in which they are sent. However, the data within a received packet would be error-free. Applications like tftp(Trivial File Transfer Protocol) work on UDP. tftp ensures data consistency by using a confirmation mechanism: receiver sends a confirmation that message is received, the absence of whose arrival within a certain interval leads to resending of the ip packet.
