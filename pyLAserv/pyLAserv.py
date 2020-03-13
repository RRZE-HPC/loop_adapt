#!/usr/bin/env python3


import socket

HOST = "127.0.0.1"
PORT = 12121

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM);

sock.bind(HOST, PORT)

sock.listen()

conn, addr = sock.accept()
with conn:
    print("Connect from {}".format(addr))
    while True:
        data = conn.recv(1024)
        if not data: break
        conn.sendall(data)



sock.close()
