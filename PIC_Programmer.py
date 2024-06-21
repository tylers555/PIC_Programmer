import argparse

import serial
import serial.tools.list_ports
import time

CONFIG1_ADDRESS  = 0x8007
NUM_CONFIG_WORDS = 5

NUM_PROGRAMMING_WORDS = 2048

arduino = None

def send_command(command):
    TIMEOUT_COUNT = 1000
    arduino.write(bytes(command, 'utf-8'))
    for i in range(0, TIMEOUT_COUNT):
        line = arduino.readline().decode('utf-8')
        if "ok" in line:
            return True
        elif "failure" in line:
            return False

def send_and_read_command(command):
    arduino.write(bytes(command, 'utf-8'))
    result = ''
    while result == '':
        result = arduino.readline().decode('utf-8').strip()
    return result

def start_programming():
    send_command('s') # start programming
    send_command('e') # bulk erase
    send_command('p') # internally timed programming, to clear data latches
    send_command('l') # row erase, to undo programming

def stop_programming():
    send_command('f') # finish programming

def do_program_memory(programming_memory):
    # for i in range(0, 32):
    #     programming_memory[i] = 0x1234

    NUM_LATCHES = 32
    for i in range(0, NUM_PROGRAMMING_WORDS, NUM_LATCHES):
        command = 'b ' + str(i) + "."
        has_actual_data = False
        for j in range(0, NUM_LATCHES):
            if programming_memory[i+j] != 0x3fff:
                has_actual_data = True
        
        if not has_actual_data:
            continue

        for j in range(0, NUM_LATCHES):
            address = i + j
            command += str(programming_memory[address]) + ','
            # if programming_memory[address] != 0x3fff:
            #     send_command(f'wa {programming_memory[address]}')
            #     print(f"writing data {address}")
            # else:
            #     send_command(f'i')

        # send_command(f'g {i}')
        # send_command('p')

        print(f"Programming: {hex(i)}")
        if not send_command(command):
            print(f"programming error with program memory! failed to write: row: {hex(i)}")

def do_config_words(config_words):
    command = "c "
    for config in config_words:
        command += str(config) + ','
    
    print(f"Programming config words")
    if not send_command(command):  
        print("Programming error with config words!")


def swap_endianness(x):
    return int.from_bytes(x.to_bytes(2, byteorder='little'), byteorder='big', signed=False)

def parse_file(file_path):
    config_words = [0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff]
    program_words = [0x3fff for i in range(0, NUM_PROGRAMMING_WORDS)]
    offset_address = 0

    with open(file_path) as file:
        for line in file:
            line = line.rstrip()
            if line[0] != ':':
                continue
            
            num_bytes = int(line[1:3], 16)
            address = int(line[3:7], 16) + offset_address
            address = int(address/2)
            record_type = int(line[7:9], 16)
            data = line[9:-2]
            checksum = line[-2:]

            if record_type == 0:
                # print(f"{hex(address)}({num_bytes}): {data}")
                if CONFIG1_ADDRESS <= address <= CONFIG1_ADDRESS+NUM_CONFIG_WORDS:
                    for i in range(int(num_bytes/2)):
                        config_words[i + (address-CONFIG1_ADDRESS)] = swap_endianness(int(data[4*i:4*i+4], 16))
                else:
                    for i in range(int(num_bytes/2)):
                        word = swap_endianness(int(data[4*i:4*i+4], 16))
                        word_address = address + i
                        program_words[word_address] = word

            elif record_type == 1:
                start_programming()
                do_program_memory(program_words)
                do_config_words(config_words)
                stop_programming()
            elif record_type == 2:
                pass
            elif record_type == 3:
                pass
            elif record_type == 4:
                offset_address = int(data, 16) << 16
            elif record_type == 5:
                pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("filename", help=".hex file to upload")
    parser.add_argument("-c", "--com", help="Override COM port")

    args = parser.parse_args()
    
    file_path = args.filename

    ports = list(serial.tools.list_ports.comports())
    com_name = args.com
    if not com_name and len(ports) > 0:
        com_name = ports[0].device

    print(f"Attempting to connect to: {com_name}")
    try:
        arduino = serial.Serial(com_name, baudrate=115200, timeout=.1, 
                                bytesize=serial.EIGHTBITS, 
                                parity=serial.PARITY_NONE, 
                                stopbits=serial.STOPBITS_ONE)
    except:
        arduino = None

    if arduino and arduino.is_open:
        while True:
            line = arduino.readline().decode('utf-8')
            if "READY" in line:
                break

        print("programming")
        parse_file(file_path)
        print("finished programming")
    else:
        print("Unable to find a valid COM port!")