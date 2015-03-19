/*
 This file is part of JustGarble.

    JustGarble is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JustGarble is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JustGarble.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "../include/justGarble.h"
#include "../include/garble.h"
#include "../include/tcpip.h"
#ifdef __cplusplus
extern "C" {
#endif


int check_test_seq(int *input, int *outputs, int nc)
{
	int n = 3;
	int m = 2;
	int w[3];
	w[1] = 0; // DFF reset
	for(int cc=0;cc<3;cc++)
	{
		int x = input[cc*n+0];
		int y = input[cc*n+1];
		w[0] = x & y;
		w[1] = w[1] ^ w[0];
		outputs[cc*m + 0] = w[1];
		outputs[cc*m + 1] = w[1] & x;
	}


	return 0;
}


int main(int argc, char* argv[])
{
#ifndef DEBUG
	srand( time(NULL));
	srand_sse( time(NULL));
#else
	srand(0);
	srand_sse(1000);
#endif

	GarbledCircuit garbledCircuit;
	long i, j, cid;


	if(argc < 4)
	{
		printf( "Usage: %s <scd file name> <ip of server> <port> \n", argv[0]);
		return -1;
	}

	int port = atoi(argv[3]);
	int sockfd = client_init(argv[2], port);
	if (sockfd == -1)
	{
		printf( "Something's wrong with the socket!");
		return -1;
	}

	readCircuitFromFile(&garbledCircuit, argv[1]);
	
	int n = garbledCircuit.n;
	int g = garbledCircuit.g;
	int p = garbledCircuit.p;
	int m = garbledCircuit.m;
	int c = garbledCircuit.c;
	int e = n - g;

	int *evalator_inputs = (int *)malloc(sizeof(int)*(e)*c);
	block *inputLabels = (block *)malloc(sizeof(block)*n*c);
	block *initialDFFLable = (block *)malloc(sizeof(block)*p);
	block *outputs = (block *)malloc(sizeof(block)*m*c);
	
	for(cid=0;cid<c;cid++)
	{
		for (j = 0; j < e; j++)
		{
			evalator_inputs[cid*e + j] = rand() % 2;
		}
	}


	for (cid = 0; cid < c; cid++)
	{
		for (j = 0; j < g; j++)
		{
			recv_block(sockfd, &inputLabels[n*cid+j]);
			printf("i(%ld,%ld,?)\n", cid, j);
			print__m128i(inputLabels[n*cid+j]);
		}

		for(j = 0 ; j < e; j++)
		{
			write(sockfd, &evalator_inputs[cid*e + j], sizeof(int));
			recv_block(sockfd, &inputLabels[cid * n + g + j]);

			printf("i(%ld,%ld,%d)\n", cid, j + g, evalator_inputs[cid*e + j]);
			print__m128i(inputLabels[cid * n + g + j]);
		}
	}
	printf("\n\n");

	for (j = 0; j < p; j++)
	{
		if(garbledCircuit.I[j] < g) // initial value is constant or belongs to Alice (garbler)
		{
			recv_block(sockfd, &initialDFFLable[j]);
			if(garbledCircuit.I[j] == CONST_ZERO)
				printf("dffi(%ld,%ld,0)\n", cid, j);
			else if(garbledCircuit.I[j] == CONST_ONE)
				printf("dffi(%ld,%ld,1)\n", cid, j);
			else
				printf("dffi(%ld,%ld,?)\n", cid, j);
			print__m128i(initialDFFLable[j]);
		}
		else
		{
			assert((garbledCircuit.I[j] - g >0) && (garbledCircuit.I[j] - g < e));

			write(sockfd, &evalator_inputs[garbledCircuit.I[j] - g], sizeof(int));
			recv_block(sockfd, &initialDFFLable[j]);

			printf("dffi(%ld,%ld,%d)\n", cid, j + g, evalator_inputs[garbledCircuit.I[j] - g]);
			print__m128i(initialDFFLable[j]);
			printf("\n");
		}
	}
	printf("\n\n");


	block DKCkey;
	recv_block(sockfd, &DKCkey); //receive key

	evaluate(&garbledCircuit, inputLabels, initialDFFLable, outputs, DKCkey, sockfd);

	return 0;
}

#ifdef __cplusplus
}
#endif
