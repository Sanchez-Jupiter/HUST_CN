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
	if (this->getWaitingState()) { //窗口装满，拒绝发送
		return false;
	}

	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->nextSeqNum;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	window.push_back(packetWaitingAck);//待发送Packet入队
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	//没有比nextSeqNum更早的未确认的，故为其启动计时器
	if (this->base == this->nextSeqNum) {
        pns->startTimer(SENDER, Configuration::TIME_OUT,this->base);	
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	this->nextSeqNum = (this->nextSeqNum + 1) % this->seqNumWidth;

	return true;
}

void GBNSender::receive(const Packet &ackPkt) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		int ackNumOffset = (ackPkt.acknum - this->base + this->seqNumWidth) % this->seqNumWidth;
		//如果校验和正确，并且序号是在已发送并等待确认的数据包序号列中
		if (checkSum == ackPkt.checksum && ackNumOffset < window.size()) {
			printf("发送方窗口:[");
			for (int i = 0; i < this->windowSize; i++) {
				printf("%d ", (this->base + i) % this->seqNumWidth);
			}
			printf("]\n");
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			pns->stopTimer(SENDER, this->base);		//关闭定时器
			//滑动窗口
			while (this->base != (ackPkt.acknum + 1) % this->seqNumWidth) {
				window.pop_front();
				this->base = (this->base + 1) % this->seqNumWidth;
			}
			printf("窗口滑动后:[");
			for (int i = 0; i < this->windowSize; i++) {
				printf("%d ", (this->base + i) % this->seqNumWidth);
			}
			printf("]\n");
			//开启时钟
			if (window.size() != 0) {
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
			}
		}
		else if (checkSum != ackPkt.checksum)
			pUtils->printPacket("发送方没有正确收到该报文确认,数据校验错误", ackPkt);
		else
			pUtils->printPacket("发送方已正确收到过该报文确认", ackPkt);

}

void GBNSender::timeoutHandler(int seqNum) {
	for (int i = 0; i < window.size(); i++) {
		pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", window.at(i));
		pns->sendToNetworkLayer(RECEIVER, window.at(i));			//重新发送数据包
	}
	pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//重新启动发送方定时器

}
