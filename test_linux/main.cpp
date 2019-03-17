typedef unsigned char byte;

#include "RRNetwork.h"
#define GINO 1
#define THISNODEID 0


RRNetwork network(THISNODEID,RPI_GPIO_P1_22, RPI_GPIO_P1_24);
void receive(byte*,byte,byte);

int main(int argc, char** argv) {
  // put your setup code here, to run once:
  network.cbNewMessage=receive;
  byte msg[]="ciao";
  network.write(msg,5,GINO);
  // put your main code here, to run repeatedly:
  while(true){
    if(millis() % 5000 ==0) printf("TXQlen: %u, TXCount: %u, time=%u\n",network.txqlen(),network.TXCount,millis());
    network.update(); 

  }
  
  return 1;
}

void receive(byte* msg, byte size, byte sender) {
  printf("RX:%u sender=%u size=%u msg=%s\n",millis(),sender, size,msg);

}