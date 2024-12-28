import socket
import struct
import time

def send_informer_init(sock):
    packet_type = 0x00
    computer_name = 'MyComputer'.ljust(32, '\x00')  # Padding to 32 bytes
    platform = 'Linux'.ljust(16, '\x00')  # Padding to 16 bytes
    cpu_model = 'Intel i7'.ljust(32, '\x00')  # Padding to 32 bytes
    cores = 8
    memory_gb = 16
    swap_gb = 8
    storage_gb = 512

    packet = struct.pack(
        '!B32s16s32sBHHQ',
        packet_type,
        computer_name.encode('utf-8'),
        platform.encode('utf-8'),
        cpu_model.encode('utf-8'),
        cores,
        memory_gb,
        swap_gb,
        storage_gb
    )

    print("Informer Initialization Packet:")
    print(f"Packet Type (B): {packet[:1].hex()}")
    print(f"Computer Name (32s): {packet[1:33].hex()} - {packet[1:33].decode('utf-8').strip()}")
    print(f"Platform (16s): {packet[33:49].hex()} - {packet[33:49].decode('utf-8').strip()}")
    print(f"CPU Model (32s): {packet[49:81].hex()} - {packet[49:81].decode('utf-8').strip()}")
    print(f"Cores (B): {packet[81:82].hex()} - {packet[81]}")
    print(f"Memory GB (H): {packet[82:84].hex()} - {struct.unpack('!H', packet[82:84])[0]}")
    print(f"Swap GB (H): {packet[84:86].hex()} - {struct.unpack('!H', packet[84:86])[0]}")
    print(f"Storage GB (Q): {packet[86:94].hex()} - {struct.unpack('!Q', packet[86:94])[0]}")
    print(f"Packet Hex: {packet.hex()}")

    sock.sendall(packet)

def send_system_info(sock, informer_id):
    packet_type = 0b10
    cpu_usage = 20
    memory_usage = 8  # 8 GB in bytes
    network_upload = 500  # 500 MB in bytes
    network_download = 300  # 300 MB in bytes
    disk_used = 200 # 200 GB in bytes

    # Pad informer_id to 32 bytes
    padded_informer_id = informer_id.encode('utf-8').ljust(32, b'\x00')
    
    packet = struct.pack(
        '!B32sQQQQQ',  # Correct format string for 8 items
        packet_type,
        padded_informer_id,  # informer_id padded to 32 bytes
        cpu_usage,
        memory_usage,
        network_upload,
        network_download,
        disk_used
    )

    print("System Information Packet:")
    print(f"Packet Type (B): {packet[:1].hex()}")
    print(f"Informer ID (32s): {packet[1:33].hex()} - {packet[1:33].decode('utf-8').strip()}")
    print(f"CPU Usage (Q): {packet[33:41].hex()} - {struct.unpack('!Q', packet[33:41])[0]}")
    print(f"Memory Usage (Q): {packet[41:49].hex()} - {struct.unpack('!Q', packet[41:49])[0]}")
    print(f"Network Upload (Q): {packet[49:57].hex()} - {struct.unpack('!Q', packet[49:57])[0]}")
    print(f"Network Download (Q): {packet[57:65].hex()} - {struct.unpack('!Q', packet[57:65])[0]}")
    print(f"Disk Used (Q): {packet[65:73].hex()} - {struct.unpack('!Q', packet[65:73])[0]}")
    print(f"Packet Hex: {packet.hex()}")

    sock.sendall(packet)

def receive_response(sock):
    response = sock.recv(128)
    return response

def main():
    host = '127.0.0.1'
    port = 8080

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))

    print("Sending Informer Initialization Packet...")
    send_informer_init(sock)
    response = receive_response(sock)
    informer_id = response[2:34].decode('utf-8').strip('\x00')
    print("Received Informer ID:", informer_id)

    time.sleep(1)

    print("Sending System Information Packet...")
    send_system_info(sock, informer_id)
    response = receive_response(sock)
    print("Received Response:", response)

    sock.close()

if __name__ == "__main__":
    main()

