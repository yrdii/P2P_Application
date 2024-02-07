# P2P-Application

For our Computer Networks course we created a Peer to Peer Application. It consists of peers who can exchange content among themselves through the index server. The communication between the index server and a peer is based on UDP and the content download is based on TCP.

## Demo

In the example below, peer1 registers several files: test1.txt, test2.txt, and test3.txt. The index server receives a confirmation (R-type PDU) and makes sure there are no other peers with the same name. Then, it sent an acknowledgment (A-type PDU) to confirm the registration. Additionally, you can see that each piece of content registered by the peer has its own separate TCP socket since the Port numbers are different.

### Peer:
![image](https://github.com/pvalia/P2P-Application/assets/77172929/08005bf1-e8f1-413e-b127-dc76222fe564)

### Index Server:  
![image](https://github.com/pvalia/P2P-Application/assets/77172929/7c11fd3c-1f26-46ef-9eaf-e1be40d2e569)
