#include <SPI.h>
#include <RF24.h>
#include <LinkedList.h> // https://github.com/ivanseidel/LinkedList
#include "RRNEnvelope.h"
#define RETRYDELAY 300 // ms
#define RETRIES 10
#define RXLISTLENGHT 10
#define DEBUG



struct rxp {
    byte sender;
    byte packetID;
};

class RRNetwork {
    public:
        RRNetwork(byte nodeid, byte cepin, byte cspin);
        ~RRNetwork() {free(radio); free(outgoingPackets);};
        void begin();
        void update();
        //void write(RRNMessage *msg, byte dest, bool = true);
        void write(byte *msg, byte len, byte dest, bool = true);
        byte CDTxerr,TXCount, ACKed,RXCount,TXErrors;
        byte PayloadTooShortErr;
        void (*cbNewMessage)(byte *, byte, byte);
        byte txqlen();
    protected:
        LinkedList<RRNEnvelope*>* outgoingPackets;
        RF24 *radio;
        byte thisNodeId;
        byte packetID;
        void processIncomingData(RRNEnvelope *rxenv);
        void processOutgoingPackets();
        RRNEnvelope* findInList(byte packetId);
        void enqueue(RRNEnvelope *env);
        struct rxp rxlist[RXLISTLENGHT];
};

byte RRNetwork::txqlen() {
    return outgoingPackets->size();
}



RRNetwork::RRNetwork(byte nodeid, byte cepin, byte cspin) {
    //rxp=malloc(sizeof(rxp)*RXLISTLENGHT));
    for (byte i=0;i<RXLISTLENGHT;i++)
    {
        rxlist[i].sender=255;
        rxlist[i].packetID=255;
    };

    thisNodeId=nodeid;
    packetID=0;
    cbNewMessage=0;
    outgoingPackets=new LinkedList<RRNEnvelope*>;
    radio=new RF24(cepin,cspin);
}

void RRNetwork::begin() {
    byte addresses[6] = {"123"};
    radio->begin();
    radio->setAddressWidth(3);
    radio->openWritingPipe(addresses);
    radio->openReadingPipe(1,addresses);
    radio->enableDynamicAck();
    radio->setChannel(76);
    radio->enableDynamicPayloads();
    radio->setAutoAck(false);
    radio->setPALevel(RF24_PA_MAX);
    radio->setDataRate(RF24_250KBPS);
    radio->setCRCLength(RF24_CRC_16);
    radio->startListening();
}

/*
void RRNetwork::write(RRNMessage *msg, byte dest, bool requestACK) {
    printf("W0:%s\n",msg->ToString());
    RRNEnvelope* env=new RRNEnvelope(msg,dest,thisNodeId,packetID++);
    env->requestACK(requestACK);
    env->print();
    outgoingPackets->add(env);
    #ifdef DEBUG
    printf("W2:");
    printEnvelope(env);
    #endif
}
*/
void RRNetwork::write(byte *msg, byte len, byte dest, bool requestACK) {
    RRNEnvelope* env=new RRNEnvelope(msg, len, dest,thisNodeId,packetID++);
    env->requestACK(requestACK);
    env->isACK(false);
    env->txCount=RETRIES;
    env->txTime=millis();
    enqueue(env);
}

void RRNetwork::enqueue(RRNEnvelope *env) {
    #ifdef DEBUG
    printf("EQ:%lu:",millis());
    env->print();
    #endif
    outgoingPackets->add(env);
}
void RRNetwork::update() {
    if (radio->available()) {
        byte ps=radio->getDynamicPayloadSize();
        if(ps>=HEADERLEN) {
            byte buf[ps];
            radio->read( buf, ps); 
            //printf("R1:%u, pl=%u, pl=%s\n", millis(),ps,buf);
            RRNEnvelope rxenv(buf,ps);
            processIncomingData(&rxenv);

        } else PayloadTooShortErr++;
    }
    
    processOutgoingPackets();

}

void RRNetwork::processIncomingData(RRNEnvelope *rxenv) {
    // if the incoming packet is an ACK delete from the list of outgoing packets
    // even if the acked packet is mine or not
    #ifdef DEBUG
    printf("R1:%lu:",millis());
    rxenv->print();
    #endif
    byte listsize=outgoingPackets->size();
    bool isack=rxenv->isACK();
    byte recipient=rxenv->recipient();
    byte sender=rxenv->sender();
    byte pid=rxenv->packetID();
    if(isack) {
        // ack for me
        // delete from list (if present) the packet that originated this ack 
        if(recipient==thisNodeId) {
            for(byte i=0;i<listsize;i++) {
                RRNEnvelope *p=outgoingPackets->get(i);
                if(p->packetID()==pid && p->sender()==thisNodeId && p->recipient()==sender) {
                    outgoingPackets->remove(i); 
                    delete(p);
                    ACKed++;
                    #ifdef DEBUG
                    printf("R11:deleted pid=%u\n",pid);
                    #endif
                    return;
                }
            }
        }
        // ack but not for me
        // delete from list (if present) the packet that originated this ack because the recipient had surely received it
        // retrasmit the ack packet
        else {
            // delete originating packet
            for(byte i=0;i<listsize;i++) {
                RRNEnvelope *p=outgoingPackets->get(i);
                if(p->packetID()==pid && p->sender()==recipient && p->recipient()==sender) {
                    outgoingPackets->remove(i); 
                    delete(p);
                    #ifdef DEBUG
                    printf("R12:deleted pid=%u\n",pid);
                    #endif
                }
            }
            // see if it's already in list
            for(byte i=0;i<listsize;i++) {
                RRNEnvelope *p=outgoingPackets->get(i);
                if(p->packetID()==pid && p->sender()==sender && p->recipient()==recipient) return;
            }
            // reenqueue
            RRNEnvelope* fm=new RRNEnvelope(recipient,sender,pid);
            fm->isACK(true);
            fm->txCount=1;
            fm->txTime=rand() % 20 + 5 + millis();
            enqueue(fm);
            #ifdef DEBUG
            printf("R121\n");
            #endif

        }
    }
    else {
        // is a standard packet and it's for me
        if(recipient==thisNodeId) {
            // if I already have processed this packet just discard it
            for(byte i=0;i<RXLISTLENGHT;i++)
            {
                if (rxlist[i].sender==sender && rxlist[i].packetID==pid) {
                    #ifdef DEBUG
                    printf("R-ALR\n");
                    #endif
                    return;
                }
            }
            rxlist[RXCount % RXLISTLENGHT].sender=sender;
            rxlist[RXCount % RXLISTLENGHT].packetID=pid;
            RXCount++;
            // ok, I don't have it, pass to the application
            if (cbNewMessage) (*cbNewMessage)(rxenv->message(),rxenv->msgsize(),rxenv->sender());
            // generate ack if is needed
            if(rxenv->requestACK()) {
                RRNEnvelope* ackmsg=new RRNEnvelope(sender,thisNodeId,pid);
                ackmsg->isACK(true);
                ackmsg->txCount=1;
                ackmsg->txTime=millis();
                enqueue(ackmsg);
                #ifdef DEBUG
                printf("R21\n");
                #endif
            }
            
        }
        // is a standard packet but not for me
        // enqueue if it's not already in list
        else {
            // test to see if it's already in list
            for(byte i=0;i<listsize;i++) {
                RRNEnvelope *p=outgoingPackets->get(i);
                if(p->packetID()==pid && p->sender()==sender && p->recipient()==recipient) return; 
            }
            // ok I don't have it: reenqueue
            RRNEnvelope* msg=new RRNEnvelope(rxenv);
            msg->txCount=1;
            msg->txTime=rand() % 20 + 5 + millis();
            enqueue(msg);
            #ifdef DEBUG
            printf("R22\n");
            #endif

        }
    }
}

void RRNetwork::processOutgoingPackets() {
    int ql=outgoingPackets->size();
    for(byte i=0; i<ql;i++) {
        RRNEnvelope* e=outgoingPackets->get(i);
        if(millis() >= e->txTime) {
            #ifdef DEBUG
            printf("T1:%lu:QL=%d: ",millis(),ql);
            e->print();
            #endif
            radio->stopListening();
            /*
            radio->stopListening();
            radio->startListening();
            delay(1);
            if(!radio->testRPD()) {
            if(1) {
            */
            radio->write(e->data(),e->size(),1);
            TXCount++;
            radio->startListening();
            if(e->txCount==1) 
            {
                if(e->sender()==thisNodeId && !e->isACK()) TXErrors++;
                #ifdef DEBUG
                printf("TXQ:deleted pid=%u\n",e->packetID());
                #endif
                delete outgoingPackets->remove(i);
            }
            else
            {
                e->txTime=(millis() + rand() % 100 + 2*RETRYDELAY*(RETRIES + 1 - e->txCount));
                e->txCount--;
            }
            //} else CDTxerr++;
            break;
        } 
    }
}

RRNEnvelope* RRNetwork::findInList(byte packetId) {
    for(byte i=0; i<outgoingPackets->size();i++) {  
        RRNEnvelope* e=outgoingPackets->get(i);
        if(e->packetID()==packetId) return e;
    }
    return nullptr;
}

