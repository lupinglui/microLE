from serial import Serial
from serial.tools.list_ports import comports
from struct import unpack

PREAMBLE = 0xBC
HEADERSIZE = 4

class CMD(object):
    TEST = 0
    SEND = 1
    RECV = 2

class DeviceError(Exception):
    def __init__(self, message):
        self._message = message
        super().__init__(self)

    def __str__(self):
        return "<DeviceError message='%s'" % self._message

    def __repr__(self):
        return str(self)


class Link(object):
    def __init__(self, interface=None, baudrate=115200, idx=None):

        # Pick the first serial port that matches a Micro:Bit
        if interface is None and idx is None:
            for port in comports():
                if type(port) is tuple:
                    if "VID:PID=0d28:0204" in port[-1]:
                        interface = port[0]
                        break
                elif port.vid == 0x0D28 and port.pid == 0x0204:
                    interface = port.device
                    break

        # TODO FIX THIS! This is unsafe !!!
        if interface is None and idx is not None:
            port = comports()[idx]
            if type(port) is tuple:
                if "VID:PID=0d28:0204" in port[-1]:
                    interface = port[0]
            elif port.vid == 0x0D28 and port.pid == 0x0204:
                interface = port.device

        if interface is None:
            raise DeviceError('No Micro:Bit connected')

        self.interface = Serial(interface, baudrate, timeout=0)
        self.rx_buffer = bytes()

    def __write(self, packet):
        return self.interface.write(bytes(packet))#.toBytes())

    def __read(self):
        self.rx_buffer += self.interface.read()

    def __async_read(self):
        self.__read()

        # ensure first byte start with 0xBC
        if len(self.rx_buffer) <= 0:
            return None

        # seek for valid preamble
        if self.rx_buffer[0] != PREAMBLE:
            try:
                pkt_start = self.rx_buffer.index(0xbc)
                self.rx_buffer = self.rx_buffer[pkt_start:]
            except ValueError:
                self.rx_buffer = bytes()

        # check if we got a valid packet
        if len(self.rx_buffer) < HEADERSIZE:
            return None

        size = unpack('<H', self.rx_buffer[2:4])[0]
        # check if we got a complete packet
        if len(self.rx_buffer) < (size + HEADERSIZE + 1):
            return None

        # parse packet
        packet = self.rx_buffer[:size + HEADERSIZE + 1]
        self.rx_buffer = self.rx_buffer[size + HEADERSIZE + 1:]
        cmd = packet[1] & 0xF
        flags = (packet[1] & 0xF0) >> 4
        payload = packet[HEADERSIZE:-1]
        crcok = self.crc(packet[:-1]) == packet[-1]
        return cmd, flags, size, payload, crcok

    def write_packet(self, cmd, flags, payload):
        print(payload)
        size = len(payload)
        buf = [
                PREAMBLE,
                (cmd & 0xF) | ((flags & 0xF) << 4),
                size & 0xFF,
                (size & 0xFF00) >> 8
            ]

        for p in payload:
            buf.append(p)

        print(buf)
        buf.append(self.crc(buf))

        self.__write(buf)

    def read_packet(self):
        packet = None
        while packet is None:
            packet = self.__async_read()
        return packet

    def crc(self, data, previous=0xff):
        c = previous
        for i in list(data):
            c ^= i
        return c
