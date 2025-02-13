#include "stdafx.h"
#include "Global.h"
#include "SRReceiver.h"


SRReceiver::SRReceiver():expectSequenceNumberRcvd(0), seqNumWidth(8), base(0), windowSize(4)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	for (int i = 0; i < windowSize; i++) {
		RcvBuffer tmp;
		tmp.flag = false;
		tmp.windowPacket.seqnum = -1;
		window.push_back(tmp);
	}
}


SRReceiver::~SRReceiver()
{
}

void SRReceiver::receive(const Packet &packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);
	int seqnumOffset = (packet.seqnum - this->base + this->seqNumWidth) % this->seqNumWidth;
	if (checkSum == packet.checksum && seqnumOffset < this->windowSize && window.at(seqnumOffset).flag ==false) {
		window.at(seqnumOffset).flag = true;
		window.at(seqnumOffset).windowPacket = packet;
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
		printf("���ܷ�����:[");
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
		

		//ȡ��Message�����ϵݽ���Ӧ�ò�
		while (window.front().flag) {
			Message msg;
			memcpy(msg.data, window.front().windowPacket.payload, sizeof(window.front().windowPacket.payload));
			pns->delivertoAppLayer(RECEIVER, msg);
			this->base = (this->base + 1) % seqNumWidth;
			RcvBuffer tmp;
			tmp.flag = false;
			tmp.windowPacket.seqnum = -1;
			window.pop_front();
			window.push_back(tmp);
		}
		printf("���ܷ������󴰿�:[");
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

		lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
		}
		else {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų��ڷ�Χ��", packet);
			lastAckPkt.acknum = packet.seqnum; 
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ���Ȼ����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢���ϴε�ȷ�ϱ���
		}
		
	}
}