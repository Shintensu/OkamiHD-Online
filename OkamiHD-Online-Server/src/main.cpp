#include <iostream>
#include <vector>
#include <enet/enet.h>

#include "wk.h"
#include "cParts/cModel/cObj/cObjBase/pl/pl00.h"

#define MAX_PLAYER_COUNT 3

struct PlayerPacket
{
    char username[32] = {};
    int mapID = 0;
    char localPlayer[0x15FC] = {};
};
PlayerPacket playerList[MAX_PLAYER_COUNT] = {};

void BroadCastPlayerList(ENetHost* host, const PlayerPacket* data)
{
    /* Create a reliable packet of size 7 containing "packet\0" */
    ENetPacket* packet = enet_packet_create(data,
        sizeof(PlayerPacket) * MAX_PLAYER_COUNT,
        ENET_PACKET_FLAG_RELIABLE);
    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    enet_host_broadcast(host, 2, packet);

    // MapID 4 byte int
    // position and rotation 4 floats, 16 bytes
    // animation data 4 byte int
}

void SendText(ENetPeer* peer, const char* data)
{
    /* Create a reliable packet of size 7 containing "packet\0" */
    ENetPacket* packet = enet_packet_create(data,
        strlen(data) + 1,
        ENET_PACKET_FLAG_RELIABLE);
    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    enet_peer_send(peer, 0, packet);
}

int main(int argc, char** argv)
{
    // Initializing ENet
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }

    // Creating a server
    ENetAddress address;
    ENetEvent event;
    ENetHost* server;
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    address.port = 54310;
    server = enet_host_create(&address /* the address to bind the server host to */,
        MAX_PLAYER_COUNT      /* allow up to MAX_PLAYER_COUNT clients and/or outgoing connections */,
        3      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);
    if (server == NULL)
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }

    // Server Loop
    while (true)
    {
        enet_host_service(server, &event, 0);


        /* Wait up to 10 milliseconds for an event. */
        while (enet_host_service(server, &event, 1) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                unsigned int tempInt;
                unsigned char* temp;
                tempInt = event.peer->address.host;
                temp = (unsigned char*) &tempInt;
                printf("A new client connected from %u.%u.%u.%u:%u.\n",
                    temp[0],
                    temp[1],
                    temp[2],
                    temp[3],
                    event.peer->address.port);
                /* Store any relevant client information here. */
                std::cout << event.peer->incomingPeerID << std::endl;
                playerList[event.peer->incomingPeerID];
                event.peer->data = &(playerList[event.peer->incomingPeerID]);

                break;
            case ENET_EVENT_TYPE_RECEIVE:
                // Check if it is a player packet
                if (event.channelID == 1)
                {
                    memcpy(event.peer->data, event.packet->data, event.packet->dataLength);

                    PlayerPacket* tempPlayer = (PlayerPacket*) event.peer->data;

                    std::cout << "Package received from Player: " << tempPlayer->username << std::endl;
                    std::cout << std::endl;
                }
                else
                {
                    printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet->dataLength,
                        event.packet->data,
                        event.peer->data,
                        event.channelID);
                }
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", event.peer->data);

                // Clear Client's PlayerPacket Object from PlayerList
                /**(PlayerPacket*)(event.peer->data) = PlayerPacket();*/
                /* Reset the peer's client information. */
                event.peer->data = NULL;
            }
        }
        BroadCastPlayerList(server, playerList);
    }



    // Cleanup
    enet_host_destroy(server);
    atexit(enet_deinitialize);
}