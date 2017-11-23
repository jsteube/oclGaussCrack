oclGaussCrack
=============

Crack the verification hash of the encrypted payload of the Gauss Virus

* Name....: oclGaussCrack
* Version.: v1.4
* Autor...: Jens Steube <jens.steube@gmail.com>
* License.: GPLv2
* Language: C, OpenCL

Why
-------------

The goal of the program is to Crack the verification hash of the encrypted payload of the Gauss Virus as described here: https://www.securelist.com/en/blog/208193781/The_Mystery_of_the_Encrypted_Gauss_Payload

* Uses OpenCL to accelerate the 10k MD5 loop
* Uses optimizations also used in oclHashcat-plus for maximum performance
* Able to handle multi-GPU setups (of the same type)
* VCL (Virtual CL) v1.17 compatible
* Open Source
* Supports integration into distributed computing environments
* Supports resume

Install
-------------

Make sure you have OpenCL enabled driver installed

If you want to compile it from sources:

Make sure to have an OpenCL SDK installed
Just run "make" to compile

Performance
-------------

    AMD Radeon HD 7970              (GPU)   489k c/s
    AMD Radeon HD 6990              (GPU)   670k c/s
    AMD FX 8120                     (CPU)    14k c/s
    AMD FX 6100                     (CPU)    11k c/s
    NVIDIA GeForce GTX 470          (GPU)    94k c/s
    NVIDIA GeForce GTX 560Ti        (GPU)    86k c/s

Intel CPU supporting OpenCL should work as well.

You can increase the performance slightly by increasing the workload GPU_ACCEL in oclGaussCrack.c.
To enable CPU mode change DEV_TYPE to CL_DEVICE_TYPE_CPU.

How to use
-------------

* The program waits for any arbitrary input data on stdin. This is your password / path / the unknown key.
* It then appends the fixed salt to the input and processes the first MD5 on CPU.
* The resulting digest is used as input for the 10k MD5 loops which is done on the GPU.
* The hashes are compared on GPU. If they match, the GID which matched is stored in the result buffer.
* The host program reads the result buffer and if the hash was cracked it uses the GID to lookup the original plaintext used.

Example
-------------

This is the official Kaspersky test-vector!

NOTE: To enable "test mode", go to src/oclGaussCrack.cl and uncomment the target hash for the test vector in lines 31 - 37.

    $ ./oclGaussCrack64.bin < test.txt

This should output:

    Loading Kernel...
    Initializing OpenCL...
    Found new device # 0: Tahiti, 32 compute units
    Initialization done, accepting candidates from stdin...
    
    
    GPU # 0: ALARM! Candidate number 0 cracked the hash! Hex dump following:
    
    43 00 3a 00 5c 00 44 00
    6f 00 63 00 75 00 6d 00
    65 00 6e 00 74 00 73 00
    20 00 61 00 6e 00 64 00
    20 00 53 00 65 00 74 00
    74 00 69 00 6e 00 67 00
    73 00 5c 00 6a 00 6f 00
    68 00 6e 00 5c 00 4c 00
    6f 00 63 00 61 00 6c 00
    20 00 53 00 65 00 74 00
    74 00 69 00 6e 00 67 00
    73 00 5c 00 41 00 70 00
    70 00 6c 00 69 00 63 00
    61 00 74 00 69 00 6f 00
    6e 00 20 00 44 00 61 00
    74 00 61 00 5c 00 47 00
    6f 00 6f 00 67 00 6c 00
    65 00 5c 00 43 00 68 00
    72 00 6f 00 6d 00 65 00
    5c 00 41 00 70 00 70 00
    6c 00 69 00 63 00 61 00
    74 00 69 00 6f 00 6e 00
    7e 00 64 00 69 00 72 00
    31 00


This is how the test.txt file has been generated:

    $ echo -n "C:\Documents and Settings\john\Local Settings\Application Data\Google\Chrome\Application~dir1" | iconv -f utf8 -t utf16le > test.txt

Parameters
-------------

oclGaussCrack supports two optional parameters since v1.2.

* The first one sets an offset that defines how many entries from input data to skip.
* The second one sets an limitation that defines how many entries from input data to process.

By doing this, you can pick a range of the workload coming on stdin.
This is required to integrate oclGaussCrack into distributed computing services, like boinc or so.
It is also useful if you want to continue your work at a specific point after it stopped for whatever reason.

Both parameters are just numbers. You can set the first one without setting the second one but not vice versa.

Limitations
-------------

* Maximum input length of the string can be configured by changing MAX_LINELEN in program header.
* Maximum number of GPUs suported can be configured by changing MAX_GPU in the program header.

gaussFilter
-------------

This tool simply skips all lines from a given input which must be encoded in utf16 in case the first character value <= 0x007a.
It is useful since gauss filters all inputs from "%PROGRAMFILES%\*" where cFileName[0] > 0x007A (UNICODE 'z')
By applying this filter we can reduce the keyspace to search a lot.

Example usage: 
    
    ./gaussFilter64.bin PROGRAMFILES.dict > PROGRAMFILES_filtered.dict

gaussCombinator
-------------

This tool simply concatinates two input sources encoded in utf16 in memory.
It is useful since there are two input sources used in gauss to generate the key.

Example usage: 

    ./gaussCombinator64.bin GetEnvironmentVariableW.dict PROGRAMFILES_filtered.dict | ./gaussCrack64.bin


--

USE AT YOUR OWN RISK. This software is provided 'AS IS' without any express or implied warranty of any kind.

Copyright (c) 2012-2017, Jens Steube
