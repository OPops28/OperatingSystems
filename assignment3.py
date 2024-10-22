import socket
import threading
import time
import sys
import argparse
from concurrent.futures import ThreadPoolExecutor

# Lock to ensure safe access to the shared list between threads
shared_list_lock = threading.Lock()

# Condition variable used to signal new data availability
new_data_condition = threading.Condition()

# Shared list to store incoming data from all clients
shared_list = []

# Function to handle client connections
def handle_client(connection, address, book_number):
    print(f"New connection: {address}")
    book = []  # Local list to store the received lines for this client
    buffer = ""  # Buffer to hold incomplete data

    while True:
        try:
            # Receive data from client in chunks of 1024 bytes
            data = connection.recv(1024).decode('utf-8')
            if not data:
                break  # If no data, client has closed the connection

            buffer += data

            # Split received data by newline to process complete lines
            lines = buffer.split('\n')

            # Process all complete lines except the last, which may be incomplete
            for i in range(len(lines) - 1):
                line = lines[i].strip()  # Strip whitespace
                if line:  # If the line is not empty
                    with shared_list_lock:
                        # Append line along with its book number to the shared list
                        shared_list.append({'book_number': book_number, 'line': line})
                    book.append(line)  # Add the line to the client's local book
                    print(f"Received line from {address}: {line}")

            # The last part of the buffer might be incomplete, so keep it
            buffer = lines[-1]

            # Notify the pattern analysis thread that new data is available
            with new_data_condition:
                new_data_condition.notify_all()

        except socket.error:
            break  # Exit the loop if there is a socket error

    # Once the connection is closed, save the collected lines to a file
    with open(f'book_{book_number:02d}.txt', 'w') as f:
        for line in book:
            f.write(line + '\n')

    print(f"Connection {address} closed, book saved as book_{book_number:02d}.txt")
    connection.close()

# Function to start the server and listen for client connections
def start_server(port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', port))  # Bind to all interfaces on the specified port
    server_socket.listen(5)  # Listen for incoming connections (up to 5 in backlog)
    print(f"Server listening on port {port}")

    book_number = 0  # Counter to assign unique book numbers for each client

    # Use a thread pool to handle multiple clients concurrently
    with ThreadPoolExecutor(max_workers=10) as executor:
        while True:
            connection, address = server_socket.accept()  # Accept new client connection
            book_number += 1
            # Submit the client handler to the thread pool
            executor.submit(handle_client, connection, address, book_number)

# Function to analyze the shared list for a specific pattern
def pattern_analysis(pattern, interval):
    while True:
        # Wait for notification that new data has been added
        with new_data_condition:
            new_data_condition.wait()

        # Wait for the specified interval before analyzing data
        time.sleep(interval)
        print(f"\n--- Searching for pattern '{pattern}' ---")
        count = 0
        # Lock the shared list while searching for the pattern
        with shared_list_lock:
            for node in shared_list:
                if pattern in node['line']:
                    count += 1
        print(f"Pattern '{pattern}' found {count} times.\n")

# Main entry point of the script
if __name__ == "__main__":
    # Argument parser to get the listening port and the pattern to search for
    parser = argparse.ArgumentParser(description="Multi-threaded network server")
    parser.add_argument('-l', '--listen', type=int, required=True, help='Port to listen on')
    parser.add_argument('-p', '--pattern', type=str, required=True, help='Pattern to search for in text')
    args = parser.parse_args()

    port = args.listen  # Get the listening port from the arguments
    pattern = args.pattern  # Get the search pattern from the arguments

    # Start the pattern analysis in a separate thread
    analysis_thread = threading.Thread(target=pattern_analysis, args=(pattern, 5))
    analysis_thread.start()

    # Start the server to listen for connections
    start_server(port)

