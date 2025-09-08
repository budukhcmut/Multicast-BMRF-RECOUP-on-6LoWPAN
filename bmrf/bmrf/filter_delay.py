# -*- coding: utf-8 -*-

import re
import numpy as np

def parse_log_file(log_file):
    time_start = {}
    delay = {}
    packet_received = {}
    total_packets = 0

    with open(log_file, 'r') as file:
        for line in file:
            # Tìm thời gian và ID
            timestamp_match = re.match(r"(\d+):(\d+).(\d+)\s+ID:(\d+)", line)
            if timestamp_match:
                min = int(timestamp_match.group(1))
                sec = int(timestamp_match.group(2))
                ms = int(timestamp_match.group(3))
                node_id = int(timestamp_match.group(4))
                timestamp = min * 60 * 1000 + sec * 1000 + ms  # milliseconds

                # Gửi gói tin
                match_send = re.search(r'Send to: .*msg=0x([0-9a-fA-F]+)', line)
                if match_send and node_id == 1:  # Chỉ gửi từ Root
                    msg_id = match_send.group(1)
                    time_start[msg_id] = timestamp
                    total_packets += 1

                # Nhận gói tin
                match_recv = re.search(r'In: sequence-msg \[(\d+)\]', line)
                if match_recv:
                    msg_index = int(match_recv.group(1))
                    msg_id = f"{msg_index:08x}"  # Hex chuỗi dài 8 ký tự
                    if node_id not in delay:
                        delay[node_id] = []
                        packet_received[node_id] = 0
                    if msg_id in time_start:
                        delay[node_id].append(timestamp - time_start[msg_id])
                        packet_received[node_id] += 1

    return delay, packet_received, total_packets

def compute_metrics(delay, packet_received, total_packets):
    results = {}
    for node_id in delay:
        avg_delay = np.mean(delay[node_id]) if delay[node_id] else 0
        pdr = (packet_received[node_id] / total_packets) * 100 if total_packets > 0 else 0
        results[node_id] = {'Average Delay': avg_delay, 'PDR': pdr}
    return results

def write_output(results, output_file):
    with open(output_file, 'w') as file:
        file.write("Node ID\tAverage Delay (ms)\tPDR (%)\n")
        for node_id, metrics in sorted(results.items()):
            file.write("{}\t{:.2f}\t{:.2f}\n".format(
                node_id, metrics['Average Delay'], metrics['PDR']
            ))
            print("Node ID {}: Average Delay = {:.2f} ms, PDR = {:.2f}%".format(
                node_id, metrics['Average Delay'], metrics['PDR']
            ))

if __name__ == "__main__":
    log_file = "loglistener 75% broadcast.txt"        # Đổi tên theo log bạn xuất
    output_file = "50%_broadcast_output.txt"        # Kết quả

    delay, packet_received, total_packets = parse_log_file(log_file)
    results = compute_metrics(delay, packet_received, total_packets)
    write_output(results, output_file)

