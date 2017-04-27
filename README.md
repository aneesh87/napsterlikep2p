Studentid: agupta27 Studentid: tchuang3

To compile run make.

To run server:
=================================================
./pserver \<port number to listen for connections>

Example:

./pserver 7890

To run client:
=================================================
./pclient \<server ip> \<server port>

Example:

./pclient 127.0.0.1 7890

Note: for localhost server ip can be 127.0.0.1

Things to note:
=================================================

1) A peer moves to server Active Registered List, only after he has sent his first ADD message to the server.

2) The time to transfer large rfc files could take 4-5 minutes.

3) If more than one peer are there for a rfc, the user is given an option to pick the peer.

4) When a registered client exists, it moves to server inactive list. It remains there until it reopens connection to server from same ip and same port no.

5) Tested on VCL Ubuntu 14.04 Base images.

