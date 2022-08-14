[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-f059dc9a6f8d3a56e377f745f24479a46679e63a5d9fe6f495e02850cd0d8118.svg)](https://classroom.github.com/online_ide?assignment_repo_id=7409126&assignment_repo_type=AssignmentRepo)
# <p align="center">PA4: Exploring IPC Methods<p>

**Introduction**

The goal of this programming assignment is to compare performance of various IPC methods during the exchange of large volumes of data between the client and the server.

PA1 used the pre-written FIFORequestChannel class that implemented a mechanism called “named pipes” or “FIFOs” to facilitate client-server IPC communication. In this programming assignment, we will be introducing 2 additional IPC mechanisms: Message Queues and Shared Memory. We will also be exploring the use of Kernel Semaphores in Shared Memory for synchronization. Each IPC mechanism has its own uses that make it suited to particular applications. 

**Tasks**

- [ ] Implement the client with provided FIFORequestChannel class
  - [ ] single datapoint request
  - [ ] multiple datapoint request (with option to use multiple channels)
  - [ ] file transfer request (with option to use multiple channels)
- [ ] Write MQRequestChannel
  - [ ] use mq_attr struct in conjunction with mq_open(...)
- [ ] Write SHMRequestChannel
  - [ ] including SHMQueue class

See the PA4 module on Canvas for further details and assistance.
