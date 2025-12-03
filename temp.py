#!/usr/bin/env python3

def check_for_bug():
    with open('new_log.txt', 'r') as debug_hd:
        number = None
        for line in debug_hd:
            if line.startswith("[UbLdstApi RecvDataPacket] Send Ack. NodeId: 1 PacketUid: "):
                length = len("[UbLdstApi RecvDataPacket] Send Ack. NodeId: 1 PacketUid: ")
                line_truncated = line[length:]
                number_new = int(line_truncated.split(' ')[0])
                if number is None or number_new == number + 1:
                    number = number_new
                    continue
                print(number)
                print(number_new)
                break;

if __name__ == "__main__":
    check_for_bug() 