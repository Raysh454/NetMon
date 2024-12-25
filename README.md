# NetMon
NetMon will help monitor system information for different computers over the network and broadcast the information to clients that are authorized to view it.

# Components

## Server

A server is the endpoint that receives system information from clients, and relay them to clients authorized for viewing the said system information

## Informer

Sends system Information to the server at an interval. Informer receives a unique code (called `Informer_ID`) after initial setup, this will be sent with each system information payload to verify the identity of client.

## Overseer

Receives System information from the server at an interval after authentication. The server then keeps the connection in memory and sends information about all the connected clients at an interval. If connection is lost, the socket is discarded and the overseer will need to re-authenticate.

#### Overseer Authentication

This will be done using a fixed password set while initializing the server, anyone with the password can connect and receive information.

![image](https://github.com/user-attachments/assets/9b1a8581-dfe8-4eb1-876d-44de3d6299fb)
