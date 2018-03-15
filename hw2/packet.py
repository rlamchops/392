from construct import *

#--------------------------------------------------------------------------------
#-------------------------Example use of construct-------------------------------
#--------------------------------------------------------------------------------
# s = Struct(
#         "a" / Byte,
#         "b" / Short,
# )
#
# print (s.parse(b"\x01\x02\x03"))

#--------------------------------------------------------------------------------
#-----------------------------Formatters for MAC & IP addresses------------------
#--------------------------------------------------------------------------------

ip4Address = ExprAdapter(Byte[4],
    decoder = lambda obj,ctx: "{0}.{1}.{2}.{3}".format(*obj),
    encoder = lambda obj,ctx: [int(x) for x in obj.split(".")],
)

ip6Address = ExprAdapter(Byte[16],
    decoder = lambda obj,ctx: ":".join("%02x" % b for b in obj),
    encoder = lambda obj,ctx: [int(part, 16) for part in obj.split(":")],
)

macAddress = ExprAdapter(Byte[6],
    decoder = lambda obj,ctx: "-".join("%02x" % b for b in obj),
    encoder = lambda obj,ctx: [int(part, 16) for part in obj.split("-")],
)

#--------------------------------------------------------------------------------
#----------------------------Packet types----------------------------------------
#--------------------------------------------------------------------------------

ethernet = Struct (
        "destination" / macAddress,
        "source" / macAddress,
        "type" / Enum(Int16ub, IPv4=0x0800, IPv6=0x86DD, default=Pass,),
)

# print (ethernet.parse(b"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x08\x00"))

ipv4 = Struct (
    "information" / BitStruct(
        "version" / Nibble,
        "headerLength" / Nibble,
    ),
    "dcsp_ecn" / BitStruct(
        "precendence" / BitsInteger(3),
        "minimize_delay" / Flag,
        "high_throughput" / Flag,
        "high_reliability" / Flag,
        "minimize_cost" / Flag,
        Padding(1),
    ),
    "total_length" / Int16ub,
    "identification" / Int16ub,
    "flags_offset" / BitStruct (
        "flags" / BitsInteger(3),
        "fragment_offset" / BitsInteger(13),
    ),
    "ttl" / Int8ub,
    "protocol" / Enum(Int8ub, TCP=0x06, UDP=0x11, default=Pass,),
    "checksum" / Int16ub,
    "source" / ip4Address,
    "destination" / ip4Address,
    #variable options field. the headerLength tells you total size of the header (in 4 byte words).
    #according to wikipedia, options only exist if headerLength > 5
    "options" / Bytes(this.information.headerLength - 5),
    #then data comes after that
)

ipv6 = Struct (
    "information" / BitStruct (
        "version" / Nibble,
        "trafficClass" / Octet,
        "flowLabel" / BitsInteger(20),
    ),
    "payloadLength" / Int16ub,
    #nextHeader has the same function as the protocol field did in IPv4
    "nextHeader" / Enum(Int8ub, TCP=0x06, UDP=0x11, default=Pass,),
    "hopLimit" / Int8ub,
    "sourceAddress" / Bytes(16),
    "destinationAddress" / Bytes(16),
    #data comes after
)


dns = Struct (
    "ID" / Int16ub,
    "Flags"/ BitStruct(
        "QR" / Flag,
        "Opcode" / BitsInteger(4),
        "AA" / Flag,
        "TC" / Flag,
        "RD" / Flag,
        "RA" / Flag,
        "Z" / BitsInteger(3), #Three reserved bits that are zero
        "RCode" / BitsInteger(4),
    ),
    "QDCount" / Int16ub,
    "ANCount" / Int16ub,
    "NSCount" / Int16ub,
    "ARCount" / Int16ub,
)

tcp = Struct (
    "sourcePort" / Int16ub,
    "destinationPort" / Int16ub,
    "seq" / Int32ub,
    "ack" / Int32ub,
    "flags" / BitStruct (
        "dataOffset" / Nibble,
        "reserved" / BitsInteger(3),
        "ns" / Flag,
        "cwr" / Flag,
        "ece" / Flag,
        "urg" / Flag,
        "ack" / Flag,
        "psh" / Flag,
        "rst" / Flag,
        "syn" / Flag,
        "fin" / Flag,
    ),
    "windowSize" / Int16ub,
    "checksum" / Int16ub,
    "urgentPointer" / Int16ub,
    #variable options field. similar to IPv4's headerLength, except dataOffset determines its size instead
    "options" / Bytes(this.flags.dataOffset - 5),
    #then data comes after that
)

udp = Struct (
    "sourcePort" / Int16ub,
    "destinationPort" / Int16ub,
    "length" / Int16ub,
    "checksum" / Int16ub,
    #data comes after
    "DNS" / dns,
)

icmp = Struct (
    "type" / Int8ub,
    "code" / Int8ub,
    "checksum" / Int16ub,
    "roh" / Int32ub,
)

#icmp and icmpv6 differ in their codes and types
icmpv6 = Struct (
    "type" / Int8ub,
    "code" / Int8ub,
    "checksum" / Int16ub,
    "roh" / Int32ub,
)
#--------------------------------------------------------------------------------
#--------------------Total packet formation--------------------------------------
#--------------------------------------------------------------------------------

layer4tcp = Struct (
    "header" / tcp,
)

layer4udp = Struct (
    "header" / udp,
)

layer4icmp = Struct (
    "header" / icmp,
)

layer4icmpv6 = Struct (
    "header" / icmpv6,
)

#the first arg of the switch is what to switch on
layer3ipv4 = Struct (
    "header" / ipv4,
    "next" / Switch(this.header.protocol, {
        "TCP": layer4tcp,
        "UDP": layer4udp,
    },default=None),
)

layer3ipv6 = Struct (
    "header" / ipv6,
    "next" / Switch(this.header.protocol, {
        "TCP": layer4tcp,
        "UDP": layer4udp,
    },default=None),
)

layer2ethernet = Struct (
    "header" / ethernet,
    "next" / Switch(this.header.type, {
        "IPv4": layer3ipv4,
        "IPv6": layer3ipv6,
    },default=None),
)

#--------------------------------------------------------------------------------
#----------------------Utility functions for sniffles----------------------------
#--------------------------------------------------------------------------------

#if option is none, then no filter, else filter
def parsePacket(packet):
    pkt = layer2ethernet.parse(packet)
    print(pkt)
    # temp = pkt.header
    # while temp:
    #     printInfo(temp)
    #     # print("hi")
    #     if "next" in pkt and pkt["next"] is not None:
    #         pkt = pkt.next
    #         if pkt is not None:
    #             temp = pkt.header
    #             # print("hi")
    #     else:
    #         break

def printInfo(header):
    print("hi")
    #is this ethernet?
    # if header.type and header.destination and header.source:
    #     print ("Ethernet(Source=%s, Destination =%s, Type=%s)" % (header.source, header.destination, header.type))
    # #is this IPv4?
    # elif header.protocol:
    #     temp = buildFlagString(header, "IPv4")
    #     print ("IPv4(Version=%s, HeadLen=%s, Precedence=%s, Dcsp/Ecn=%s, TotalLen=%s, Identification=%s, Flags=%s, FragmentOffset=%s, Ttl=%s, Protocol=%s, Chksum=%s, Src=%s, Dest=%s, Options=%s)" % (header.information.version, header.information.headerLength, header.dcsp_ecn.precendence, temp, header.total_length, header.identification, header.flags_offset.flags, header.flags_offset.fragment_offset, header.ttl, header.protocol, header.checksum, header.source, header.destination, header.options))
    # #is this IPv6?
    # elif header.nextHeader:
    #     print("IPv6(Version=%s, TrafficClass=%s, FlowLabel=%s, PayloadLength=%s, NextHeader=%s, HopLimit=%s, SrcAdd=%s, DestAddr=%s)" % (header.information.version, header.information.trafficClass, header.information.flowLabel, header.payloadLength, header.nextHeader, header.hopLimit, header.sourceAddress, header.destinationAddress))
    # #is this tcp?
    # elif header.ack:
    #     temp = buildFlagString(header,"TCP")
    #     print("TCP(SrcPort=%s, DestPort=%s, SeqNum=%s, AckNum=%s, DataOff=%s, Flags=%s, WinSize=%s, ChkSum=%s, UrgPointer=%s, Options=%s)" % (header.sourcePort, header.destinationPort, header.seq, header.ack, header.flags.dataOffset, temp, header.windowSize, header.checkSum, header.urgentPointer, header.options))
    # #if there's no ack, then is this udp?
    # elif header.sourcePort and header.destinationPort:
    #     print("UDP(SrcPort=%s, DestPort=%s, Length=%s, Chksum=%s)" % (header.sourcePort, header.destinationPort, header.length, header.checksum))
    # else:
    #     print ("Unknown/unsupported packet type")


def buildFlagString(header, type):
    ret = "{"
    if type=="TCP":
        if header.flags.ns:
            ret+="NS "
        if header.flags.cwr:
            ret+="CWR "
        if header.flags.ece:
            ret+="ECE "
        if header.flags.urg:
            ret+="URG "
        if header.flags.ack:
            ret+="ACK "
        if header.flags.psh:
            ret+="PSH "
        if header.flags.rst:
            ret+="RST "
        if header.flags.syn:
            ret+="SYN "
        if header.flags.cwr:
            ret+="FIN"
    elif type=="IPv4":
        if header.dcsp_ecn.minimize_delay:
            ret+="MinDelay "
        if header.dcsp_ecn.high_throughput:
            ret+="HiTPut "
        if header.dcsp_ecn.high_reliability:
            ret+="HiRelia "
        if header.dcsp_ecn.minimize_cost:
            ret+="MinCost"
    ret += "}"
    return ret