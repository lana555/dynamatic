//
// Created by pejic on 09/07/19.
//

#include "MyChannel.h"
#include "MyPort.h"
#include "DFnetlist.h"

MyChannel::MyChannel() : id{-1} {};

MyChannel::MyChannel(DFnetlist& df, int channel_id)
{
	readFromDF(df, channel_id);
}

MyChannel::MyChannel(DFnetlist & df, int srcPortId, int dstPortId) :
		srcPort(srcPortId),
		dstPort(dstPortId),
		slots(0),
		transparent(true) {
	id = df.DFI->createChannel(srcPort, dstPort, slots, transparent);
	writeToDF(df);
}

void MyChannel::readFromDF(DFnetlist& df, int channelId)
{
	id = channelId;
	srcPort = df.getSrcPort(channelId);
	dstPort = df.getDstPort(channelId);
	//slots = df.getChannelBufferSize(channelId);
	transparent = df.isChannelTransparent(channelId);
}

void MyChannel::writeToDF(DFnetlist &df)
{
	df.DFI->setChannelTransparency(id, transparent);
	df.DFI->setChannelBufferSize(id, slots);
}
