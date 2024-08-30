import numpy as np

# Open the binary file
with open('dataset.bin', 'rb') as file:
    # Record the file size
    file.seek(0, 2)  # Seek to the end of the file to get its size
    file_size = file.tell()  # Get the current position, which is the size of the file
    file.seek(0)  # Seek back to the beginning of the file
    
    # Read the first 4 bytes as an integer
    int_value = np.frombuffer(file.read(4), dtype=np.int32)[0]

    # Read the next 'int_value' number of integers
    int_array = np.frombuffer(file.read(int_value * 4), dtype=np.int32)

    num_floats = 1
    for i in int_array:
        num_floats *= i

    # Read the next 'int_array' number of floats
    float_array = np.frombuffer(file.read(num_floats * 4), dtype=np.float32)
    float_array = float_array.reshape(int_array)

    # Check if the file has been fully read
    bytes_read = file.tell()  # Get the current position of the file pointer
    
    if bytes_read == file_size:
        print("The whole file has been read.")
    else:
        print(f"Not all data was read. {file_size - bytes_read} bytes remain.")

print(int_value)
print(int_array)
print(float_array.shape)
