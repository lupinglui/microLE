from link import Link, CMD
import time

usb_link = Link(idx=1)

#CMD, FLAGS, PKT
#usb_link.write_packet(0xff, 5, b"THIS IS A TEST")
#usb_link.write_packet(CMD.SEND, 0, b"BLAH")
usb_link.write_packet(CMD.TEST, 5, b"TEST")
#usb_link.write_packet(CMD.RECV, 0, b"")
usb_link.write_packet(CMD.SEND, 0, b"")
usb_link.write_packet(CMD.TEST, 5, b"TEST")
while True:
    print(usb_link.read_packet())
    time.sleep(0.1)
