import socket
import threading
import time
import sys
import argparse
from concurrent.futures import ThreadPoolExecutor

shared_list_lock = threading.Lock()
new_data_condition = threading.Condition()

shared_list = []

def handle_client(connection, address, book_number):
    print(f"New connection: {address}")
    book = []
    buffer = "" 
    while True:
        try:
            data = connection.recv(1024).decode('utf-8')
            if not data:
                break

            buffer += data

            lines = buffer.split('\n')
            
            for i in range(len(lines) - 1):
                line = lines[i].strip() 
                if line: 
                    with shared_list_lock:
                        shared_list.append({'book_number': book_number, 'line': line})
                    book.append(line)
                    print(f"Received line from {address}: {line}")

            buffer = lines[-1]

            with new_data_condition:
                new_data_condition.notify_all()

        except socket.error:
            break

    with open(f'book_{book_number:02d}.txt', 'w') as f:
        for line in book:
            f.write(line + '\n')
    print(f"Connection {address} closed, book saved as book_{book_number:02d}.txt")
    connection.close()

def start_server(port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', port))  
    server_socket.listen(5)
    print(f"Server listening on port {port}")

    book_number = 0

    with ThreadPoolExecutor(max_workers=10) as executor:
        while True:
            connection, address = server_socket.accept()
            book_number += 1
            executor.submit(handle_client, connection, address, book_number)

def pattern_analysis(pattern, interval):
    while True:
        with new_data_condition:
            new_data_condition.wait()

        time.sleep(interval)
        print(f"\n--- Searching for pattern '{pattern}' ---")
        count = 0
        with shared_list_lock:
            for node in shared_list:
                if pattern in node['line']:
                    count += 1
        print(f"Pattern '{pattern}' found {count} times.\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Multi-threaded network server")
    parser.add_argument('-l', '--listen', type=int, required=True, help='Port to listen on')
    parser.add_argument('-p', '--pattern', type=str, required=True, help='Pattern to search for in text')
    args = parser.parse_args()

    port = args.listen
    pattern = args.pattern

    analysis_thread = threading.Thread(target=pattern_analysis, args=(pattern, 5))
    analysis_thread.start()

    start_server(port)
