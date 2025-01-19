import subprocess
import filecmp
import random
import string
import os
from concurrent.futures import ThreadPoolExecutor

def run_application(source, destination, shared_memory):
    output = subprocess.run(['../bin/copyTool.exe', 
                    '--source', source, 
                    '--destination', destination, 
                    '--shared_memory', shared_memory], 
                    check=True, stdout=subprocess.PIPE, text=True)
    print(output.stdout)

def generate_large_file(filename, size_in_mb):
    size = size_in_mb * 1024 * 1024
    chars = ''.join(random.choices(string.ascii_letters + string.digits, k=1024))
    with open(filename, 'w') as f:
        for _ in range(0, size, 1024):
            f.write(chars) 

def main():
    sourceFile = './source'
    generate_large_file(sourceFile, 1024)
    destinationFile = '../destination'
    shared_memory = 'sm1'
    
    with ThreadPoolExecutor(max_workers=2) as executor:
        futures = []
        futures.append(executor.submit(run_application, sourceFile, destinationFile, shared_memory))
        futures.append(executor.submit(run_application, sourceFile, destinationFile, shared_memory))
        
        for future in futures:
            future.result()
    
    if filecmp.cmp(sourceFile, destinationFile, shallow=False):
        print("Test Passed: The files are the same.")
    else:
        print("Test Failed: The files are different.")

    os.remove(destinationFile)
    os.remove(sourceFile)

if __name__ == "__main__":
    main()