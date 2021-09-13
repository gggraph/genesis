

          _
        ,/_\,
      ,/_/ \_\,
     /_/ ___ \_\
    /_/ |(V)| \_\
      |  .-.  |                                    _____________
      | / / \ |                                 //              \\
      | \ \ / |                                 |  G E N E S I S ||
      |  '-'  |                                 |                ||
      '--,-,--'                                 |    C H A I N   ||
         | |                                    \\______________//
         | |
         | |
         /\|
         \/|
          /\
          \/



# Genesis Blockchain

## Overview

Genesis Blockchain is a **proof-of-work blockchain** for local network. 

It can run **smartcontracts** through a virtual machine similar to x86 microprocessor ( instruction set is reduced for blockchain
purposes). 
A custom assembler is provided, langage syntax is similar to NASM.

Genesis Blockchain parameters are easily tweakable and have been originally made to experiment mining process.

An _untitled cryptocurrency_ is also implemented, following same dynamics as Bitcoin software (coin halving, mining reward...etc.)

All is written in C-style for low-end hardware compatibility. 

## Build and Run

Genesis was made for Windows OS. You can compile it importing all .cpp and .h files located in /src directory using the IDE of your choice.
A pre-built executable is located in /src/build for windows console app.  
A MONO version which can be used on Linux system can be found at . 
An atmega-328 port is currently in development. 

## First step and network configuration

Once your executable is built. Place it in an empty folder, many files will be generated at first start. 
Make sure _net.ini_ file is located at the root directory of the executable. It contains important information about
all your peers IP. Genesis Blockchain only run on local environment even if its design was purposed
to meet larger and public network. You can manually set IP:port for both starting a server and connecting to other
machines, simply by editing the file. It is by default configured to local address 127.0.0.1 for testing purpose.

## About file structure 

The Genesis Blockchain software will create directory and files at its root. Let's see them one by one : 

##### The blockchain files

* /blockchain is a directory containing all _official_ blocks. There are blocks that has been validated and has won the distance rule : 
          they are fully integrated to the blockchain. 
* All blocks are stored and named in an ascending way. /blockchain/0 is a file containing genesis block and all future blocks up to 
  4 294 967 295  bytes ( originally designed to be written on FAT32 system). /blockchain/1 containing all next official blocks ... etc. 
* /blockchain/blocksptr is a file containing data pointing to block files and byte offsets of all official blocks. It is used to easily
  retrieving block data without reading the whole blockchain.  

##### The competitive blocks files 

* /fork is a directory contaning all competitive blocks. Genesis Blockchain _upgrade_ process happened every time the last index of 
  of competitive blocks reached or going above **last official block index + 6** ( 6 is an arbitrary number which can be changed inside
          params.h ).
* During mining process, the software will start craft next block from longest competitive blocks file. Mining strategy is up to you. 
* Competitive blocks are named with the hexadecimal representation of latest block hash in its file. 
          
##### The UTXO files 

* /utxos is a directory containing all _wallets_ at the current state of the official blockchain. In the same way for block files, they are named
  in an ascending order. /utxos/0 containing first wallets which has been registered during blocks validation. 
  utxos files are overwritten when upgrading the official blockchain. 
* UTXO stand for Unspend Transaction Output. An Utxo is a data structure
  containing public key of 64b, a sold ( or amount of coin hold by the key) and a token called Token of Uniqueness ( or TOU ) which is burnt
  at every new transaction of the wallet holder to avoid double spending problem. 
          
##### The Temporary storage

* /tmp folder contains all files which have been downloaded server-side. 
* DLL files are both pending transaction data or blocks data waiting to be validated. 
* Files are systematically deleted after their process. So consider them as _cache_ files. 
          
##### The Pending Transactions File

* located at /ptx. This file contains all transaction which has been received by peers or built with command console.
* ECDSA verification is always done to transactions before being appended to this file. 
* When Genesis Software prepare mining, it fills the new block with transaction inside /ptx file. 
* /ptx file is refreshed every time blockchain upgraded, deleting those who have old TOU or those who have purished. 
          

## Write Smart Contract

Now, we will going to technical material. 
The Genesis Blockchain treat actually with three types of transaction. 
First one is **DFT** (Default Transaction), which is a basic exchange of coin between two wallets. 
Second is called **CST** (Contract Submission Transaction), this transaction contains a series of instructions for the Genesis Virtual Machine. 
A CST has a generic header transaction data (public key pointer, secp256k1 signature, a t.o.u. and a purishmment time) but his specific data is a number of entries pointing to memory address and a long list of bytecode. When a CST is validated inside a block, its whole list of instructions can be called by the following type of transaction during the whole blockchain lifetime by any user:
A **CRT** ( Contract Request Transaction ) is a transaction that run a smartcontract code at specific offset. Technically, it will load the compiled smart contract, push some data to the virtual machine stack if necessary, then perform a jump at a given memory address. Just thinking, CRT fit x86 calling conventions. 

To both write CRT and CST for the Genesis Blockchain, run GenesisExplorer.exe (which can be downloaded at /src/build). It looks like this:  

![alt text](https://github.com/gggraph/genesis/blob/main/TRANSACTION%20VIEWER%20B.png)

##### Write a simple CST code

You can write your contract assembly code inside left textbox element (the big one). 
Just to make things clear, the _genesis vm_ can work with 8086 original instruction set. It has all general registers and a flag register which is not fully implemented yet (only ZF is used) . 
Smart Contract is loaded at **0x5D address of the vm memory**. VM memory allocation size is limited to **32kb**. Stack going downward. Registers depth is 32-bit.
Genesis vm acts as a CISC microprocessor. You will meet again MOD-REG-RM, SIB mode, direct and indirect addressing, 8-bit or 32-bit displacement... etc.
The syntax to follow to convert your code to binary is similar to nasm x86 code. 
**Last but not least, the instruction set is limited to blockchain usage. VM can only read and write data within its memory or its contract storage file. 
Pseudo-randomness, listening ports or anything from the outside cannot be achieved!** 

So let's start with a basic example code : 
          
          var1 DD 20
          var2 DD 0
          
          start:
          MOV EAX, [var1]
          MOV EBX, 8
          ADD EAX, EBX
          MOV dword[var1+4], EAX
          CMP dword[var2], 28
          JE END
          MOV ECX, 1
          JMP start
          
          end:
          HLT
