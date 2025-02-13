#include "stdafx.h"
#include "Global.h"
#include "SRSender.h"
#include <deque>

SRSender::SRSender():nextSeqNum(0),waitingState(false), base(0), windowSize(4), seqNumWidth(8)
{
}


SRSender::~SRSender()
{
}



bool SRSender::getWaitingState() {
	waitingState = (window.size() == windowSize);
	return waitingState;
}




bool SRSender::send(const Message &message) {
	if (this->getWaitingState()) { //����װ�����ܾ�����
		return false;
	}

	this->packetWaitingAck.acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck.seqnum = this->nextSeqNum;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	PckBuffer tmpPkt;
	tmpPkt.flag = false;
	tmpPkt.windowPacket = packetWaitingAck;
	window.push_back(tmpPkt);//������Packet���
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck);
	//Ϊÿ����������ʱ��
    pns->startTimer(SENDER, Configuration::TIME_OUT, this->nextSeqNum);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	this->nextSeqNum = (this->nextSeqNum + 1) % this->seqNumWidth;

	return true;
}

void SRSender::receive(const Packet &ackPkt) {
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		int ackNumOffset = (ackPkt.acknum - this->base + this->seqNumWidth) % this->seqNumWidth;
		//���У�����ȷ��������������ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ��������
		if (checkSum == ackPkt.checksum && ackNumOffset < window.size() && window.at(ackNumOffset).flag == false) {
			pns->stopTimer(SENDER, ackPkt.acknum);
			window.at(ackNumOffset).flag = true;
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
			printf("���ͷ�����:[");
			for (int i = 0; i < this->windowSize; i++) {
				printf("%d", (this->base + i) % seqNumWidth);
				if (i < window.size()) {
					if (window.at(i).flag)
						printf("Y ");
					else printf("N ");
				}
				else printf(" ");
			}
			printf("]\n");
			

			//��������
			while (window.size() != 0 && window.front().flag   ) {
				window.pop_front();
				this->base = (this->base + 1) % this->seqNumWidth;
			}
			printf("���ڻ�����:[");
			for (int i = 0; i < this->windowSize; i++) {
				printf("%d", (this->base + i) % seqNumWidth);
				if (i < window.size()) {
					if (window.at(i).flag)
						printf("Y ");
					else printf("N ");
				}
				else printf(" ");
			}
			printf("]\n");
		}
		else if (checkSum != ackPkt.checksum)
			pUtils->printPacket("���ͷ�û����ȷ�յ��ñ���ȷ��,����У�����", ackPkt);
		else
			pUtils->printPacket("���ͷ�����ȷ�յ����ñ���ȷ��", ackPkt);

}

void SRSender::timeoutHandler(int seqNum) {
	int ackNumOffset = (seqNum - this->base + this->seqNumWidth) % this->seqNumWidth;
	pns->stopTimer(SENDER,seqNum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", window.at(ackNumOffset).windowPacket);
	pns->sendToNetworkLayer(RECEIVER, window.at(ackNumOffset).windowPacket);			//���·������ݰ�
											

}
