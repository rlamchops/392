from construct import *

#--------------------------------------------------------------------------------
#-------------------------Example use of construct-------------------------------
#--------------------------------------------------------------------------------
s = Struct(
        "a" / Byte,
        "b" / Short,
)

print (s.parse(b"\x01\x02\x03"))

#--------------------------------------------------------------------------------
#----------------------------Packet types----------------------------------------
#--------------------------------------------------------------------------------

ethernet = Struct (
        "destination" / Bytes(6),
        "source" / Bytes(6),
        "type" / Enum(Int16ub, IPv4=0x0800, ARP=0x0806, RARP=0x8035, X25=0x0805, IPX=0x8137, IPv6=0x86DD,),
)

print (ethernet.parse(b"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x08\x00"))

ipv4 = Struct (
    BitStruct(
        "version" / Nibble,
        "headerLength" / Nibble,
    ),
    BitStruct(
        "precendence" / BitsInteger(3),
        "minimize_delay" / Flag,
        "high_throughput" / Flag,
        "high_reliability" / Flag,
        "minimize_cost" / Flag,
        Padding(1),
    ),
    "total_length" / Int16ub,
    "identification" / Int16ub,
    BitStruct (
        "flags" / BitsInteger(3),
        "fragment_offset" / BitsInteger(13),
    ),
    "ttl" / Int8ub,
    "protocol" / Enum(Int8ub, TCP=0x06, UDP=0x11,),
    "checksum" / Int16ub,
    "source" / Int32ub,
    "destination" / Int32ub,
    #variable options field, then data comes after that
)

ipv6 = Struct (
    BitStruct (
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
    BitStruct (
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
    #variable options field, then data comes after that
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
    Embedded(tcp),
)

layer4udp = Struct (
    Embedded(udp),
)

#the first arg of the switch is what to switch on. currently a placeholder
layer3ipv4 = Struct (
    Embedded(ipv4),
    Switch(this.protocol, {
        "TCP": layer4tcp,
        "UDP": layer4udp,
    }),
)

layer3ipv6 = Struct (
    Embedded(ipv6),
    Switch(this.protocol, {
        "TCP": layer4tcp,
        "UDP": layer4udp,
    }),
)

layer2ethernet = Struct (
    Embedded(ethernet),
    Switch(this.type, {
        "IPv4": layer3ipv4,
        "IPv6": layer3ipv6,
    }),
)