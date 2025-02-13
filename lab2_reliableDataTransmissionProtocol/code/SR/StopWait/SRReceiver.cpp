#include "stdafx.h"
#include "Global.h"
#include "SRReceiver.h"


SRReceiver::SRReceiver():expectSequenceNumberRcvd(0), seqNumWidth(8), base(0), windowSize(4)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
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
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);
	int seqnumOffset = (packet.seqnum - this->base + this->seqNumWidth) % this->seqNumWidth;
	if (checkSum == packet.checksum && seqnumOffset < this->windowSize && window.at(seqnumOffset).flag ==false) {
		window.at(seqnumOffset).flag = true;
		window.at(seqnumOffset).windowPacket = packet;
		pUtils->printPacket("接收方正确收到发送方的报文", packet);
		printf("接受方窗口:[");
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
		

		//取出Message，向上递交给应用层
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
		printf("接受方滑动后窗口:[");
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

		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不在范围内", packet);
			lastAckPkt.acknum = packet.seqnum; 
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方仍然发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送上次的确认报文
		}
		
	}
}