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
			::psn::float3 pos;
			::psn::float3 ori;
			::psn::float3 speed;
			::psn::float3 accel;
			::psn::float3 target;
			int id;
			float pos_ty = 0.0;
			for (int i = 0; i < cinput->numSamples; i++) {
				trackers[i] = ::psn::tracker(i, "");
				for (int j = 0; j < cinput->numChannels; j++) {
					// translate
					if (std::strcmp(cinput->getChannelName(j), "tx") != 0) pos.x = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "ty") != 0) pos.y = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "tz") != 0) pos.z = cinput->getChannelData(j)[i];

					// orientation
					if (std::strcmp(cinput->getChannelName(j), "rx") != 0) ori.x = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "ry") != 0) ori.y = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "rz") != 0) ori.z = cinput->getChannelData(j)[i];

					// speed
					if (std::strcmp(cinput->getChannelName(j), "speedx") != 0) speed.x = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "speedy") != 0) speed.y = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "speedz") != 0) speed.z = cinput->getChannelData(j)[i];

					// acceleration
					if (std::strcmp(cinput->getChannelName(j), "ax") != 0) accel.x = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "ay") != 0) accel.y = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "az") != 0) accel.z = cinput->getChannelData(j)[i];

					// target
					if (std::strcmp(cinput->getChannelName(j), "targetx") != 0) target.x = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "targety") != 0) target.y = cinput->getChannelData(j)[i];
					if (std::strcmp(cinput->getChannelName(j), "targetz") != 0) target.z = cinput->getChannelData(j)[i];

					// id
					if (std::strcmp(cinput->getChannelName(j), "id") != 0) {
						id = (int)cinput->getChannelData(j)[i];
					}
					else
					{
						id = i;
					}
				}
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

