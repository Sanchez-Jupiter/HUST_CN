#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
#include <deque>
struct RcvBuffer {
	bool flag;
	Packet windowPacket;
};
class SRReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	// 期待收到的下一个报文序号
	int seqNumWidth;
	Packet lastAckPkt;				//上次发送的确认报文
	int base;
	int windowSize;                 //窗口大小
	deque<RcvBuffer> window;

public:
	SRReceiver();
	virtual ~SRReceiver();

public:
	
	void receive(const Packet &packet);	//接收报文，将被NetworkService调用
};

#endif

