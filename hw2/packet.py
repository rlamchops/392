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
        "type" / Enum(Int16ub, IPv4=0x0800, ARP=0x0806, RARP=0x8035, X25=0x0805, IPX=0x8137, IPv6=0x86DD, default=Pass,),
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
    #nextHeader has the same function as the protocol field in IPv4
    "nextHeader" / Int8ub,
    "hopLimit" / Int8ub,
    "sourceAddress" / Bytes(16),
    "destinationAddress" / Bytes(16),
    #data comes after
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

#the first arg of the switch is what to switch on. currently a placeholder
layer3ipv4 = Struct (
    "header" / ipv4,
    "layer4 " / Switch(this.header.protocol, {
        "TCP": layer4tcp,
        "UDP": layer4udp,
    }),
)

layer3ipv6 = Struct (
    "header" / ipv6,
    "layer4" / Switch(this.header.protocol, {
        "TCP": layer4tcp,
        "UDP": layer4udp,
    }),
)

layer2ethernet = Struct (
    "header" / ethernet,
    "layer3" / Switch(this.header.type, {
        "IPv4": layer3ipv4,
        "IPv6": layer3ipv6,
    }),
)

#--------------------------------------------------------------------------------
#----------------------Utility functions for sniffles----------------------------
#--------------------------------------------------------------------------------
def parsePacket(packet):
    pkt = layer2ethernet.parse(packet)
    print(pkt)