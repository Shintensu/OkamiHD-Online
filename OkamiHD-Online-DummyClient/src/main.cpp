#include <iostream>
#include <enet/enet.h>

FILE* fDummy;

int temp = 0x0;
int lastTemp = 0x0;

const int sleepTime = 50;

bool isConnected = false;

char username[32] = "Testensu";
char serverip[16] = "127.0.0.1";

struct PlayerPacket
{
	char username[32];
	int mapID;
	float x, y, z, r;
	int animationStage;
	int animationID;
};

void SendPlayerUpdate(ENetPeer* peer, const PlayerPacket* data)
{
	/* Create a reliable packet of size 7 containing "packet\0" */
	ENetPacket* packet = enet_packet_create(data,
		sizeof(PlayerPacket),
		ENET_PACKET_FLAG_RELIABLE);
	/* Send the packet to the peer over channel id 0. */
	/* One could also broadcast the packet by         */
	/* enet_host_broadcast (host, 0, packet);         */
	enet_peer_send(peer, 1, packet);

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

int main()
{
	// Alloc Console for debugging
	AllocConsole();

	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);

	printf("Username: ");
	scanf_s("%32s", username, sizeof(username));
	printf("Server IP: ");
	scanf_s("%16s", serverip, sizeof(serverip));

	// Initializing ENet
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		return EXIT_FAILURE;
	}

	// Creating a Client
	ENetHost* client;
	client = enet_host_create(NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		3 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);
	if (client == NULL)
	{
		fprintf(stderr,
			"An error occurred while trying to create an ENet client host.\n");
		exit(EXIT_FAILURE);
	}

	// Declaring some loop relevant variables
	ENetAddress address;
	ENetEvent event;
	ENetPeer* peer = nullptr;

	PlayerPacket* playerList = nullptr;

	PlayerPacket localPlayer;

	// the actual thread loop
	while (true)
	{
		// ENet client Loop
		enet_host_service(client, &event, 0);

		// Update localPlayer

			strcpy_s(localPlayer.username, username);
			localPlayer.mapID = 010101;
			localPlayer.x = 100.0f;
			localPlayer.y = 100.0f;
			localPlayer.z = 100.0f;
			localPlayer.r = 1.0f;
			localPlayer.animationID = 0xF0F0F0F0;
			localPlayer.animationStage = 0xAFAFAFAF;

		// Disconnect
		if (GetAsyncKeyState(VK_NUMPAD1) && isConnected)
		{
			enet_peer_disconnect(peer, 0);
			isConnected = false;
			while (enet_host_service(client, &event, 3000) > 0)
			{
				switch (event.type)
				{
				case ENET_EVENT_TYPE_RECEIVE:
					enet_packet_destroy(event.packet);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					puts("Disconnection succeeded.");
				}
			}
		}

		if (isConnected)
		{
			// SendText(peer, "Hello World!");

			/* Wait up to 100 milliseconds for an event. */
			while (enet_host_service(client, &event, 100) > 0)
			{
				std::cout << "Event occuring" << std::endl;
				switch (event.type)
				{
				case ENET_EVENT_TYPE_RECEIVE:
					if (event.channelID == 2)
					{
						SendPlayerUpdate(peer, &localPlayer);

						if (!playerList)
						{
							playerList = (PlayerPacket*)calloc(event.packet->dataLength / sizeof(PlayerPacket), sizeof(PlayerPacket));
						}

						playerList = (PlayerPacket*)event.packet->data;

						std::cout << localPlayer.x << " ";
						std::cout << localPlayer.y << " ";
						std::cout << localPlayer.z << std::endl;

						for (int i = 0; i < event.packet->dataLength / sizeof(PlayerPacket); i++)
						{
							if (playerList[i].username)
							{
								std::cout << "PlayerPacket " << i + 1 << std::endl;
								std::cout << playerList[i].x << " ";
								std::cout << playerList[i].y << " ";
								std::cout << playerList[i].z << std::endl;
							}
						}
						std::cout << std::endl;
					}
					else
					{
						printf("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
							event.packet->dataLength,
							event.packet->data,
							event.peer->address.host,
							event.peer->address.port,
							event.channelID);
					}
					/* Clean up the packet now that we're done using it. */
					enet_packet_destroy(event.packet);
					break;
				}
			}
		}

		// Connect/Reconnect
		if (GetAsyncKeyState(VK_NUMPAD0) && !isConnected)
		{
			/* Set address to localhost. */
			enet_address_set_host(&address, serverip);
			address.port = 54310; // 54310

			/* Initiate the connection, allocating the two channels 0 and 1. */
			peer = enet_host_connect(client, &address, 3, 0);
			if (peer == NULL)
			{
				fprintf(stderr,
					"No available peers for initiating an ENet connection.\n");
				exit(EXIT_FAILURE);
			}
			/* Wait up to 5 seconds for the connection attempt to succeed. */
			if (enet_host_service(client, &event, 5000) > 0 &&
				event.type == ENET_EVENT_TYPE_CONNECT)
			{
				puts("Connection to Server succeeded.");
				isConnected = true;
			}
			else
			{
				/* Either the 5 seconds are up or a disconnect event was */
				/* received. Reset the peer in the event the 5 seconds   */
				/* had run out without any significant event.            */
				enet_peer_reset(peer);
				puts("Connection to Server failed.");
			}
		}
	}
	// Cleanup
	enet_host_destroy(client);
	enet_deinitialize();
}