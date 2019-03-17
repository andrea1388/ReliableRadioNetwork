# ReliableRadioNetwork
Classes for builing a relialable radio network for Arduino ioT

Allows to build a network of sensors/actuators based on Arduino boards equipped with cheap nRF24L01+ radio modules.
Every node of the network cooperates to ensure that the message is correctly delivered to the final recipient.
Classes manages auto retransmission and packet forwarding.

Use is simple:
1) instantiate a RRNetwork object

RRNetwork network(NODEID,RF24CEPIN,RF24CSPIN);

2) start it in setup()

```
network.begin();
```

3) keep it running in loop()

```
network.update(); 
```

4) to send a message

```
network.write(msg,MSGLEN,GATEWAY);
```

5) receive messange in callback function

```
void receive(byte* msg, byte size, byte sender) {
  printf("RX:%lu msg from %u, msgsize=%u, msg=%s\n",millis(),sender,size,msg);
}
```
