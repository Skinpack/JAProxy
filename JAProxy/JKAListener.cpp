#include "JKAListener.h"

#include <iostream>

#include <JKAProto/protocol/ClientPacket.h>
#include <JKAProto/protocol/ServerPacket.h>
#include <JKAProto/protocol/State.h>
#include <JKAProto/protocol/PacketEncoder.h>
#include <JKAProto/protocol/Netchan.h>
#include <JKAProto/packets/AllConnlessPackets.h>
#include <JKAProto/packets/ConnlessPacketFactory.h>

JKA::Huffman JKAListener::globalHuff{};

std::future<bool> JKAListener::startLoop()
{
    return std::async(std::launch::async, [this]() {
        return pcap.startLoopIPBlocking([this](const PcapPacket & packet) { packetArrived(packet); }).isSuccess();
    });
}

void JKAListener::breakLoop()
{
    pcap.breakLoop();
}

std::string JKAListener::createFilterStr()
{
    std::ostringstream filter;
    filter << "udp and ((dst " << serverAddr
        << " and dst port " << serverPort << ") or (src "
        << serverAddr << " and src port " << serverPort << "))";
    return filter.str();
}

void JKAListener::throwOnResultFail(std::string_view step, const PcapResult & res)
{
    if (!res) {
        throw JKAListenerException(step, res.errorStr());
    }
}

void JKAListener::trySetKnownDatalink(Pcap & pcapObj)
{
    if (pcapObj.isDatalinkKnown(pcapObj.getDatalink())) {
        return;
    }

    auto supportedDatalinks = pcapObj.getSupportedDatalinks();
    if (!supportedDatalinks) {
        throw JKAListenerException("getting supported datalinks", supportedDatalinks.errorStr());
    }

    for (auto it = supportedDatalinks->begin(); it != supportedDatalinks->end(); it++) {
        if (pcapObj.isDatalinkKnown(*it)) {
            throwOnResultFail("setting supported datalink", pcapObj.setDatalink(*it));
            return;
        }
    }

    throw JKAListenerException("getting supported datalinks", "no supported datalinks");
}

void JKAListener::packetArrived(const PcapPacket & packet)
{
    auto udpOpt = SimpleUdpPacket::fromRawIp(JKA::Utility::Span(packet.data));
    if (!udpOpt) JKA_UNLIKELY {
        return;  // Invalid packet
    }
    auto & udp = udpOpt.value();
    PacketDirection packetDir = getPacketDirection(udp);
    switch (packetDir) 	{
    case JKAListener::PacketDirection::FROM_CLIENT:
    {
        packetFromClient(JKA::Protocol::RawPacket(std::string(udp.data.to_sv())));
        break;
    }
    case JKAListener::PacketDirection::FROM_SERVER:
    {
        packetFromServer(JKA::Protocol::RawPacket(std::string(udp.data.to_sv())));
        break;
    }
    case JKAListener::PacketDirection::NOT_RELATED: JKA_UNLIKELY
    {
        break;
    }
    }
}