/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/
#include "psn_lib.hpp"
#include "utils/udp_socket.h"
//#include <cmath>
#include <iostream>
#include <list>
//#include <string>

#include "PosiStageSenderCHOP.h"
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <assert.h>

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
void
FillCHOPPluginInfo(CHOP_PluginInfo *info)
{
	// Always set this to CHOPCPlusPlusAPIVersion.
	info->apiVersion = CHOPCPlusPlusAPIVersion;

	// The opType is the unique name for this CHOP. It must start with a 
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Posistagenetsender");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("PSN Sender");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("tyrell");
	info->customOPInfo.authorEmail->setString("colas@tyrell.studio");

	// This CHOP can work with 0 inputs
	info->customOPInfo.minInputs = 1;

	// It can accept up to 1 input though, which changes it's behavior
	info->customOPInfo.maxInputs = 1;
}

DLLEXPORT
CHOP_CPlusPlusBase*
CreateCHOPInstance(const OP_NodeInfo* info)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per CHOP that is using the .dll
	return new PosiStageSenderCHOP(info);
}

DLLEXPORT
void
DestroyCHOPInstance(CHOP_CPlusPlusBase* instance)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the CHOP using that instance is deleted, or
	// if the CHOP loads a different DLL
	delete (PosiStageSenderCHOP*)instance;
}

};


PosiStageSenderCHOP::PosiStageSenderCHOP(const OP_NodeInfo* info) : myNodeInfo(info)
{
	socket_server.enable_send_message_multicast();
}

PosiStageSenderCHOP::~PosiStageSenderCHOP()
{

}

void
PosiStageSenderCHOP::getGeneralInfo(CHOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;

	// Note: To disable timeslicing you'll need to turn this off, as well as ensure that
	// getOutputInfo() returns true, and likely also set the info->numSamples to how many
	// samples you want to generate for this CHOP. Otherwise it'll take on length of the
	// input CHOP, which may be timesliced.
	ginfo->timeslice = true;

	ginfo->inputMatchIndex = 0;
}

bool
PosiStageSenderCHOP::getOutputInfo(CHOP_OutputInfo* info, const OP_Inputs* inputs, void* reserved1)
{
	return false;
}

void
PosiStageSenderCHOP::getChannelName(int32_t index, OP_String *name, const OP_Inputs* inputs, void* reserved1)
{
}

void
PosiStageSenderCHOP::execute(CHOP_Output* output,
							  const OP_Inputs* inputs,
							  void* reserved)
{

	// clear the warning and error vector
	errors.clear();

	if (inputs->getNumInputs() > 0)
	{
		::psn::psn_encoder psn_encoder("Touchdesigner sender");
		::psn::tracker_map trackers;
		const OP_CHOPInput* cinput = inputs->getInputCHOP(0);
		if (cinput->numChannels >= 1) {
			for (int i = 0; i < cinput->numSamples; i++) {
				int id = i;
				::psn::float3 pos;
				::psn::float3 ori;
				::psn::float3 speed;
				::psn::float3 accel;
				::psn::float3 target;
				for (int j = 0; j < cinput->numChannels; j++) {
					const char* chanName = cinput->getChannelName(j);
					float value = cinput->getChannelData(j)[i];

					if (std::strcmp(chanName, "tx") == 0) pos.x = value;
					else if (std::strcmp(chanName, "ty") == 0) pos.y = value;
					else if (std::strcmp(chanName, "tz") == 0) pos.z = value;
					else if (std::strcmp(chanName, "rx") == 0) ori.x = value;
					else if (std::strcmp(chanName, "ry") == 0) ori.y = value;
					else if (std::strcmp(chanName, "rz") == 0) ori.z = value;
					else if (std::strcmp(chanName, "speedx") == 0) speed.x = value;
					else if (std::strcmp(chanName, "speedy") == 0) speed.y = value;
					else if (std::strcmp(chanName, "speedz") == 0) speed.z = value;
					else if (std::strcmp(chanName, "ax") == 0) accel.x = value;
					else if (std::strcmp(chanName, "ay") == 0) accel.y = value;
					else if (std::strcmp(chanName, "az") == 0) accel.z = value;
					else if (std::strcmp(chanName, "targetx") == 0) target.x = value;
					else if (std::strcmp(chanName, "targety") == 0) target.y = value;
					else if (std::strcmp(chanName, "targetz") == 0) target.z = value;
					else if (std::strcmp(chanName, "id") == 0) {
						id = (int)value; // Assign id only when channel name is "id"
					}
				}

				trackers[i] = ::psn::tracker(id, "");
				trackers[i].set_pos(pos);
				trackers[i].set_ori(ori);
				trackers[i].set_speed(speed);
				trackers[i].set_accel(accel);
				trackers[i].set_target_pos(target);
				trackers[i].set_id(id);
				trackers[i].set_status(i / 10.0f);
			}

			{
				::std::list< ::std::string > data_packets = psn_encoder.encode_data(trackers, timestamp);

				::std::cout << "--------------------PSN SERVER-----------------" << ::std::endl;
				::std::cout << "Sending PSN_DATA_PACKET : "
					<< "Frame Id = " << (int)psn_encoder.get_last_data_frame_id()
					<< " , Packet Count = " << data_packets.size() << ::std::endl;

				for (auto it = data_packets.begin(); it != data_packets.end(); ++it)
				{
					// Uncomment these lines if you want to simulate a packet drop now and then
					/*static uint64_t packet_drop = 0 ;
					if ( packet_drop++ % 100 != 0 )*/
					socket_server.send_message(::psn::DEFAULT_UDP_MULTICAST_ADDR, ::psn::DEFAULT_UDP_PORT, *it);
				}

				::std::cout << "-----------------------------------------------" << ::std::endl;
			}

			if (timestamp % 100 == 0) // transmit info at 1 Hz approx.
			{
				::std::list< ::std::string > info_packets = psn_encoder.encode_info(trackers, timestamp);

				::std::cout << "--------------------PSN SERVER-----------------" << ::std::endl;
				::std::cout << "Sending PSN_INFO_PACKET : "
					<< "Frame Id = " << (int)psn_encoder.get_last_info_frame_id()
					<< " , Packet Count = " << info_packets.size() << ::std::endl;

				for (auto it = info_packets.begin(); it != info_packets.end(); ++it)
					socket_server.send_message(::psn::DEFAULT_UDP_MULTICAST_ADDR, ::psn::DEFAULT_UDP_PORT, *it);

				::std::cout << "-----------------------------------------------" << ::std::endl;
			}
		}

	}
	timestamp++;
}

int32_t
PosiStageSenderCHOP::getNumInfoCHOPChans(void * reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 0;
}

void
PosiStageSenderCHOP::getInfoCHOPChan(int32_t index,
										OP_InfoCHOPChan* chan,
										void* reserved1)
{
}

bool		
PosiStageSenderCHOP::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	return false;
}

void
PosiStageSenderCHOP::getInfoDATEntries(int32_t index,
										int32_t nEntries,
										OP_InfoDATEntries* entries, 
										void* reserved1)
{
}

void
PosiStageSenderCHOP::setupParameters(OP_ParameterManager* manager, void *reserved1)
{
}

void 
PosiStageSenderCHOP::pulsePressed(const char* name, void* reserved1)
{

}

