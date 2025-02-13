#include "stdafx.h"
#include "Global.h"
#include "GBNSender.h"
#include <deque>

GBNSender::GBNSender():nextSeqNum(0),waitingState(false), base(0), windowSize(4), seqNumWidth(8)
{
}


GBNSender::~GBNSender()
{
}



bool GBNSender::getWaitingState() {
	waitingState = (window.size() == windowSize);
	return waitingState;
}




bool GBNSender::send(const Message &message) {
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
	//û�б�nextSeqNum�����δȷ�ϵģ���Ϊ��������ʱ��
	if (this->base == this->nextSeqNum) {
        pns->startTimer(SENDER, Configuration::TIME_OUT,this->base);	
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	this->nextSeqNum = (this->nextSeqNum + 1) % this->seqNumWidth;

	return true;
}

void GBNSender::receive(const Packet &ackPkt) {
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
		else
			pUtils->printPacket("���ͷ�����ȷ�յ����ñ���ȷ��", ackPkt);

}

void GBNSender::timeoutHandler(int seqNum) {
	for (int i = 0; i < window.size(); i++) {
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", window.at(i));
		pns->sendToNetworkLayer(RECEIVER, window.at(i));			//���·������ݰ�
	}
	pns->stopTimer(SENDER,seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//�����������ͷ���ʱ��

}
