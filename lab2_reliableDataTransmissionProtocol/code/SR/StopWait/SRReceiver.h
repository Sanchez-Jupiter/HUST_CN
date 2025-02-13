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
	int expectSequenceNumberRcvd;	// �ڴ��յ�����һ���������
	int seqNumWidth;
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	int base;
	int windowSize;                 //���ڴ�С
	deque<RcvBuffer> window;

public:
	SRReceiver();
	virtual ~SRReceiver();

public:
	
	void receive(const Packet &packet);	//���ձ��ģ�����NetworkService����
};

#endif

