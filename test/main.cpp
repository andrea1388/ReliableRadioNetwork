#include <Arduino.h>
#include "RRNetwork.h"
#include "printf.h"

#define NODEID 1
#define GATEWAY 0
#define RF24CEPIN 9
#define RF24CSPIN 10


int counter;
void receive(byte*,byte,byte);
byte msg[]="hello";
RRNetwork network(NODEID,RF24CEPIN,RF24CSPIN);



void setup() {
  Serial.begin(115200);
  printf_begin();
  network.begin();
  network.cbNewMessage=receive;
  pinMode(2,INPUT_PULLUP);
  randomSeed(analogRead(0));
}

void loop() {

  network.update(); 

  // if button attached to D2 is pressed, send a packet to GATEWAY
  if(digitalRead(2)==LOW) {
    delay(100); // simple debounce
    network.write(msg,6,GATEWAY);
  }

  // every 10 seconds print network statistics
  if(millis() % 10000 ==0) {
    printf("NS:%lu, TXQlen:%u, TXCount: %u RXCount: %u TXErr: %u ACDed: %u\n",millis(),network.txqlen(),network.TXCount,network.RXCount,network.TXErrors, network.ACKed);
    delay(1);
  }
}

// when a message for this node is received this funzion is called
void receive(byte* msg, byte size, byte sender) {
  printf("RX:%lu msg from %u, msgsize=%u, msg=%s\n",millis(),sender,size,msg);

}
