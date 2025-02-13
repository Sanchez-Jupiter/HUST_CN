#include "stdafx.h"
#include "Global.h"
#include "TCPSender.h"
#include <deque>

TCPSender::TCPSender():nextSeqNum(0),waitingState(false), base(0), windowSize(4), seqNumWidth(8), AckCnt(0)
{
}


TCPSender::~TCPSender()
{
}



bool TCPSender::getWaitingState() {
	waitingState = (window.size() == windowSize);
	return waitingState;
}




bool TCPSender::send(const Message &message) {
	if (this->getWaitingState()) { //����װ�����ܾ�����
		return false;
	}

	this->packetWaitingAck.acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck.seqnum = this->nextSeqNum;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	window.push_back(packetWaitingAck);//������Packet���
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck);

	if (this->base == this->nextSeqNum) {
        pns->startTimer(SENDER, Configuration::TIME_OUT,this->base);	
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	this->nextSeqNum = (this->nextSeqNum + 1) % this->seqNumWidth;

	return true;
}

void TCPSender::receive(const Packet &ackPkt) {
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		int ackNumOffset = (ackPkt.acknum - this->base + this->seqNumWidth) % this->seqNumWidth;
		//���У�����ȷ��������������ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ��������
		if (checkSum == ackPkt.checksum && ackNumOffset < window.size()) {
			printf("���ͷ�����:[");
			for (int i = 0; i < this->windowSize; i++) {
				printf("%d ", (this->base + i) % this->seqNumWidth);
			}
			printf("]\n");
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
			pns->stopTimer(SENDER, this->base);		//�رն�ʱ��
			//��������
			while (this->base != (ackPkt.acknum + 1) % this->seqNumWidth) {
				window.pop_front();
				this->base = (this->base + 1) % this->seqNumWidth;
			}
			printf("���ڻ�����:[");
			for (int i = 0; i < this->windowSize; i++) {
				printf("%d ", (this->base + i) % this->seqNumWidth);
			}
			printf("]\n");
			//����ʱ��
			if (window.size() != 0) {
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
			}
		}
		else if (checkSum != ackPkt.checksum)
			pUtils->printPacket("���ͷ�û����ȷ�յ��ñ���ȷ��,����У�����", ackPkt);
		else if (ackPkt.acknum == (this->base - 1 + this->seqNumWidth) % this->seqNumWidth) {
			pUtils->printPacket("���ͷ�����ȷ�յ����ñ���ȷ��", ackPkt);
			this->AckCnt++;
			if (this->AckCnt == 3 && window.size() > 0) {
				pUtils->printPacket("���ͷ����������ش����ش����緢����δ��ȷ�ϵı��Ķ�", window.front());
				pns->sendToNetworkLayer(RECEIVER, window.front());
				this->AckCnt = 0;
			}
		}
			

}

void TCPSender::timeoutHandler(int seqNum) {
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ش����緢����δ��ȷ�ϵı��Ķ�", window.front());
	pns->sendToNetworkLayer(RECEIVER, window.front());			//���·������ݰ�
	pns->stopTimer(SENDER,seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//�����������ͷ���ʱ��

}
