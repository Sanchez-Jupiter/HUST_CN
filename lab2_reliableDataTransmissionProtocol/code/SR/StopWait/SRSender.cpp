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
	if (this->getWaitingState()) { //窗口装满，拒绝发送
		return false;
	}

	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->nextSeqNum;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	PckBuffer tmpPkt;
	tmpPkt.flag = false;
	tmpPkt.windowPacket = packetWaitingAck;
	window.push_back(tmpPkt);//待发送Packet入队
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	//为每个都启动定时器
    pns->startTimer(SENDER, Configuration::TIME_OUT, this->nextSeqNum);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	this->nextSeqNum = (this->nextSeqNum + 1) % this->seqNumWidth;

	return true;
}

void SRSender::receive(const Packet &ackPkt) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		int ackNumOffset = (ackPkt.acknum - this->base + this->seqNumWidth) % this->seqNumWidth;
		//如果校验和正确，并且序号是在已发送并等待确认的数据包序号列中
		if (checkSum == ackPkt.checksum && ackNumOffset < window.size() && window.at(ackNumOffset).flag == false) {
			pns->stopTimer(SENDER, ackPkt.acknum);
			window.at(ackNumOffset).flag = true;
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			printf("发送方窗口:[");
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
			

			//滑动窗口
			while (window.size() != 0 && window.front().flag   ) {
				window.pop_front();
				this->base = (this->base + 1) % this->seqNumWidth;
			}
			printf("窗口滑动后:[");
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
			pUtils->printPacket("发送方没有正确收到该报文确认,数据校验错误", ackPkt);
		else
			pUtils->printPacket("发送方已正确收到过该报文确认", ackPkt);

}

void SRSender::timeoutHandler(int seqNum) {
	int ackNumOffset = (seqNum - this->base + this->seqNumWidth) % this->seqNumWidth;
	pns->stopTimer(SENDER,seqNum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", window.at(ackNumOffset).windowPacket);
	pns->sendToNetworkLayer(RECEIVER, window.at(ackNumOffset).windowPacket);			//重新发送数据包
											

}
