//
// Created by pejic on 09/07/19.
//

#include "MyPort.h"
#include "DFnetlist.h"

MyPort::MyPort(DFnetlist& df, int port_id, int block_id)
{
	readFromDF(df, port_id, block_id);
}

MyPort::MyPort(): portId(-1){}


void MyPort::readFromDF(DFnetlist& df, int port_id, int block_Id)
{
	portId = port_id;
	name = df.getPortName(portId, false);
	blockId = block_Id;
	isInput = df.isInputPort(portId);
	width = df.getPortWidth(portId);
	delay = df.getPortDelay(portId);
	type = df.getPortType(portId);
	channelId = df.getConnectedChannel(portId);
}

void MyPort::writeToDF(DFnetlist &df)
{
	df.setPortDelay(portId, delay);
}
