//
//  main.cpp
//  Rayle-Client
//
//  Created by Jeremy Hanlon on 3/8/15.
//  Copyright (c) 2015 Rayle. All rights reserved.
//

#include <iostream>
#include <thread>
#include <list>

#define CLIENT_PORT 2631
#define SERVER_PORT 2632
#define SERVER_IP ( 127 << 24 ) |  ( 0 << 16 ) |  ( 0 << 8 ) |  1

#define MAX_PACKET_SIZE 1024

// platform detection
#define PLATFORM_WINDOWS 1 
#define PLATFORM_MAC 2 
#define PLATFORM_UNIX 3 

#if defined(_WIN32) 
    #define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__) 
    #define PLATFORM PLATFORM_MAC
#else 
    #define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS 
    #pragma comment( lib, "wsock32.lib" )
    #include <winsock2.h>
#elif PLATFORM == PLATFORM_MAC ||  PLATFORM == PLATFORM_UNIX  
    #include <sys/socket.h> 
    #include <netinet/in.h>
    #include <fcntl.h>
#endif

bool initializeSockets() {
    #if PLATFORM == PLATFORM_WINDOWS 
        WSADATA WsaData;
        return WSAStartup( MAKEWORD(2,2),  &WsaData )  == NO_ERROR;
    #else
        return true;
    #endif 
}

void shutdownSockets() {
    #if PLATFORM == PLATFORM_WINDOWS
        WSACleanup();
    #endif 
}



struct Packet {
    char* data;
    long size;
};

#define PACKET_TYPE_SIZE 2
//BEGIN: PacketBytecodeOutgoing
#define LOGIN_ATTEMPT {0, 1}
#define CREATE_ACCOUNT_ATTEMPT {10, 0}
//END: PacketBytecodeOutgoing
//BEGIN: PacketBytecodeIncoming
#define RECEIVE_MESSAGE {0, 0}
#define FAILED_LOGIN {1, 0}
    #define SUCCESSFUL_LOGIN {1, 1}
    #define LOGOUT {1, 2}
#define ENTITY_HP {2, 0}
    #define ENTITY_MAX_HP {2, 1}
    #define ENTITY_X {2, 2}
    #define ENTITY_Y {2, 3}
    #define ENTITY_Z {2, 4}
    #define ENTITY_ANIMATION_ID {2, 5}
    #define ENTITY_MODEL_IP {2, 6}
    #define ENTITY_OPTION {2, 7}
#define NEW_PLAYER {3, 0}
    #define PLAYER_NAME {3, 1}
    #define PLAYER_LEVEL {3, 2}
    #define PLAYER_LEAVE {3, 3}
#define NEW_NPC {4, 0}
    #define NPC_NAME {4, 1}
    #define NPC_LEVEL {4, 2}
    #define NPC_ID {4, 3}
    #define NPC_COMBAT_STYLE {4, 1}
    #define NPC_DIALOG {4, 1}
#define NEW_GAME_OBJECT {5, 0}
    #define GAME_OBJECT_ID {5, 1}
    #define GAME_OBJECT_X {5, 2}
    #define GAME_OBJECT_Y {5, 3}
    #define GAME_OBJECT_Z {5, 4}
    #define GAME_OBJECT_ANIMATION_ID {5, 5}
    #define GAME_OBJECT_MODEL_ID {5, 6}
    #define GAME_OBJECT_OPTION {5, 7}
#define CREATE_ACCOUNT_ERROR {10, 0}
    #define CREATE_ACCOUNT_SUCCESS {10, 1}
//END: PacketBytecodeIncoming


bool packetTypeEquals(Packet p1, char type[PACKET_TYPE_SIZE]) {
    if (p1.size < PACKET_TYPE_SIZE) {
        return false;
    }
    for (int i = 0; i < PACKET_TYPE_SIZE; i++) {
        if (p1.data[i] != type[i]) {
            return false;
        }
    }
    return true;
}

void handlePacket(Packet p) {

}


void runRecieverThread(int handle) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( SERVER_IP );
    addr.sin_port = htons( SERVER_PORT );
    while (true) {
        char packet_data[MAX_PACKET_SIZE];
        unsigned int max_packet_size =  sizeof( packet_data );
        #if PLATFORM == PLATFORM_WINDOWS
            typedef int socklen_t;
        #endif
        sockaddr_in from;
        socklen_t fromLength = sizeof( from );
        long bytes = recvfrom( handle,  (char*)packet_data,  max_packet_size, 0,  (sockaddr*)&from,  &fromLength );
        if ( bytes > 0 ) {
            Packet p;
            p.data = packet_data;
            p.size = bytes;
            handlePacket(p);
        }
        
    }
}

bool sendPacket(int handle, Packet p) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( SERVER_IP );
    addr.sin_port = htons( SERVER_PORT );
    long sent_bytes =  sendto( handle,  p.data,  p.size, 0,  (sockaddr*)&addr,  sizeof(sockaddr_in) );
    if ( sent_bytes != p.size ) {
        printf( "failed to send packet\n" );
        return false;
    }
    return true;
}

static std::list<Packet> packetsToSend;

void runSenderThread(int handle) {
    while (true) {
        Packet p = packetsToSend.front();
        packetsToSend.pop_front();
        sendPacket(handle, p);
    }
}

void addPacketToSend(Packet p) {
    packetsToSend.push_back(p);
}

int createSocket() {
    int handle = socket( AF_INET,  SOCK_DGRAM,  IPPROTO_UDP );
    if ( handle <= 0 ) {
        printf( "failed to create socket\n" );
        return -1;
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port =  htons( (unsigned short) CLIENT_PORT );
    if ( bind( handle,  (const sockaddr*) &address,  sizeof(sockaddr_in) ) < 0 ) {
        printf( "failed to bind socket\n" );
        return -1;
    }
    return handle;
}

int main(int argc, const char * argv[]) {
    if (initializeSockets() == false) {
        printf("couldn't initialize sockets\n");
        exit(0);
    }
    int handle = createSocket();
    std::thread receiver (runRecieverThread, handle);
    std::thread sender (runSenderThread, handle);
    
    
    
    receiver.join();
    sender.join();
    
    return 0;
}
