#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
#include <deque>
class TCPSender :public RdtSender
{
private:
	int nextSeqNum;	// ��һ��������� 
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	Packet packetWaitingAck;		//�ѷ��Ͳ��ȴ�Ack�����ݰ�
	int base;                       //���ڻ����
	int windowSize;                 //���ڴ�С
	int seqNumWidth;                //��ſ��
	deque <Packet> window;          //����  
	int AckCnt;
public:
	bool getWaitingState();
	bool send(const Message &message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet &ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	TCPSender();
	virtual ~TCPSender();
};

#endif

