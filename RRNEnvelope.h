/*  Evelope structure
    <byte0><byte1><byte2><byte3><message0>..<messageN>
    byte0:  bit 0 isACK
            bit 1 ACK requested
    byte1:  sender ID
    byte2:  recipient ID
    byte3:  packet ID
*/
#include <stdlib.h>
#define HEADERLEN 4
#define CONTROL 0
#define SENDER 1
#define RECIPIENT 2
#define PACKETID 3

class RRNEnvelope {
    protected:
        byte *msg;
        byte datalength;
    public:
        //RRNEnvelope(RRNMessage *, byte dest, byte sender, byte packetid);
        RRNEnvelope(byte *source, byte len, byte dest, byte sender, byte pid);
        RRNEnvelope(RRNEnvelope*);
        RRNEnvelope(const byte *, byte len);
        RRNEnvelope(byte dest, byte sender, byte pid);
        
        ~RRNEnvelope() {free (msg);};
        void requestACK(bool ra) {ra? msg[CONTROL] |= 1 : msg[CONTROL] &= 0xfe; };
        bool requestACK() {return ((msg[CONTROL] & 1) > 0);};
        void isACK(bool ia) {ia? msg[CONTROL] |= 2 : msg[CONTROL] &= 0xfd;};
        bool isACK() {return ((msg[CONTROL] & 2) > 0);};
        byte packetID() {return msg[PACKETID];};
        void packetID(byte pi) {msg[PACKETID]=pi;};
        byte sender() {return (byte)msg[SENDER];};
        byte recipient() {return (byte)msg[RECIPIENT];}
        byte* data() {return msg;};
        byte* message() {return (byte*)(&msg[HEADERLEN]);};
        byte size() {return datalength;}
        byte msgsize() {return datalength-HEADERLEN;}
        void print();
        byte txCount;
        unsigned long txTime;

};

void RRNEnvelope::print() {
    printf("ENVELOPE:size=%u:",datalength);
    for(byte i=0;i<datalength;i++)
        printf("%02X ",msg[i]);
    printf(" ");
    printf("S/D/pid %u/%u/%u, isACK=%d, wantACK=%d, msgsize=%d",sender(),recipient(),packetID(),isACK(),requestACK(), msgsize());
    if(datalength>HEADERLEN) printf(" msg=%s",message());
    printf("\n");
}
/*
RRNEnvelope::RRNEnvelope(RRNMessage *source, byte dest, byte sender, byte pid) {
    datalength=source->size() + HEADERLEN;
    msg=(byte *)malloc(datalength);
    memcpy(msg+HEADERLEN,source,source->size());
    msg[RECIPIENT]=dest;
    msg[SENDER]=sender;
    this->packetID(pid);
    this->txCount=3;
    this->txTime=millis();

}
*/
RRNEnvelope::RRNEnvelope(byte *source, byte len, byte dest, byte sender, byte pid) {
    datalength=len + HEADERLEN;
    msg=(byte *)malloc(datalength);
    memcpy(msg+HEADERLEN,source,len);
    msg[CONTROL]=0;
    msg[RECIPIENT]=dest;
    msg[SENDER]=sender;
    msg[PACKETID]=pid;
}

// copy from another Envelope
RRNEnvelope::RRNEnvelope(RRNEnvelope* e) {
    datalength=e->size();
    msg=(byte *)malloc(datalength);
    memcpy(msg,e->msg,datalength);
}

RRNEnvelope::RRNEnvelope(const byte *data, byte len) {
    datalength=len;
    msg=(byte *)malloc(datalength);
    memcpy(msg,data,len);
}

// Envelope with empty message (for ack)
RRNEnvelope::RRNEnvelope(byte dest, byte sender, byte pid) {
    datalength= HEADERLEN;
    msg=(byte *)malloc(datalength);
    msg[CONTROL]=0;
    msg[RECIPIENT]=dest;
    msg[SENDER]=sender;
    msg[PACKETID]=pid;
    this->txCount=3;
    this->txTime=millis();

}


