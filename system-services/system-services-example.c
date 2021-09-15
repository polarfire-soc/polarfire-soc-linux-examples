// SPDX-License-Identifier: MIT
/*
 * MSS system services example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFSIZE 4096

#define SERIAL_COMMAND 0u
#define SERIAL_MSG_SIZE 0u
#define SERIAL_RESP_SIZE 16u
#define SERIAL_OFFSET 0u
#define SERIAL_DEVICE "/dev/mpfs_generic_service"

#define DIGEST_COMMAND 0x4u
#define DIGEST_MSG_SIZE 0u
#define DIGEST_RESP_SIZE 576u
#define DIGEST_OFFSET 0u
#define DIGEST_DEVICE "/dev/mpfs_generic_service"
#define DIGEST_LINE_LEN 66u
#define DIGEST_SERIAL_OFFSET 50u

#define SIGN_COMMAND 26u
#define SIGN_MSG_SIZE 48u
#define SIGN_RESP_SIZE 104u
#define SIGN_OFFSET 0u
#define SIGN_READ_BYTES (3 + 2 * SIGN_RESP_SIZE) /* 3 for status and a space, 192 chars for the hex signature (96 bytes) */
#define SIGN_DEVICE "/dev/mpfs_generic_service"


static void display_output(char *data, unsigned int byte_length)
{
    unsigned int inc;
    char byte = 0;
    char buffer[BUFFSIZE];
    char *bufferp = buffer;

	for (inc = 0; inc < byte_length; inc++)
	{
		if(inc%32 == 0 && inc != 0)
		{
			bufferp += sprintf(bufferp, "\r\n");
		}
		byte = data[32*(1+inc/32) - inc%32 - 1];
		bufferp += sprintf(bufferp, "%02x", byte);
    }

    printf("%s\n", buffer);
}

int main()
{
    unsigned char inbuff[BUFFSIZE];
    unsigned char *buff = inbuff;
    unsigned int command, msg_size, resp_size, send_offset, response_offset;
    FILE *fptr;
    char chr;

    for (;/*ever*/;)
    {
        printf("PolarFire SoC system services example program.\r\nPress:\r\n");
        printf("1 - to show the FPGA device serial number\r\n2 - to show the FPGA device digests\r\n3 - continuously output random numbers from the TRNG, until ctrl+c\r\n4 - to request an ECDSA signature\r\ne - to exit this program\r\n");
        chr = getchar();
        getchar();

        switch (chr)
        {
        case '1':
            if (access(SERIAL_DEVICE, F_OK))
            {
                printf("Error! file doesnt exist\n");
                exit(1);
            }
            else if ((fptr = fopen(SERIAL_DEVICE, "w")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }

            command = SERIAL_COMMAND;
            msg_size = SERIAL_MSG_SIZE;
            resp_size = SERIAL_RESP_SIZE;
            send_offset = SERIAL_OFFSET;
            response_offset = SERIAL_OFFSET;
            fprintf(fptr, "%c%c%c%c%c%c%c%c%c",
                    command,
                    (char)msg_size, (char)(msg_size >> 8),
                    (char)resp_size, (char)(resp_size >> 8),
                    (char)send_offset, (char)(send_offset >> 8),
                    (char)response_offset, (char)(response_offset >> 8));
            fclose(fptr);
            if ((fptr = fopen(SERIAL_DEVICE, "r")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            fscanf(fptr, "%[^\n]", inbuff);
            printf("Icicle kit serial number: %s\n", inbuff);
            fclose(fptr);
            break;
        case '2':
            if (access(DIGEST_DEVICE, F_OK))
            {
                printf("Error! file doesnt exist\n");
                exit(1);
            }
            else if ((fptr = fopen(DIGEST_DEVICE, "w")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            command = DIGEST_COMMAND;
            msg_size = DIGEST_MSG_SIZE;
            resp_size = DIGEST_RESP_SIZE;
            send_offset = DIGEST_OFFSET;
            response_offset = DIGEST_OFFSET;
            fprintf(fptr, "%c%c%c%c%c%c%c%c%c",
                    command,
                    (char)msg_size, (char)(msg_size >> 8),
                    (char)resp_size, (char)(resp_size >> 8),
                    (char)send_offset, (char)(send_offset >> 8),
                    (char)response_offset, (char)(response_offset >> 8));
            fclose(fptr);
            if ((fptr = fopen(DIGEST_DEVICE, "r")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            printf("pfsoc fpga digest:\n");
            size_t ret;
            do {
                ret = fread(inbuff, 1, DIGEST_LINE_LEN, fptr);
                printf("%.*s", (unsigned int)ret, inbuff);
            } while (ret == DIGEST_LINE_LEN);
            if (feof(fptr))
            {
                printf("\n");
            }
            fclose(fptr);
            break;
        case '3':
            if ((fptr = fopen("/dev/hwrng", "r")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            for (;/*ever*/;)
            {
                fread(inbuff, 32, 1, fptr);
                display_output(inbuff, 32);
            }
            fclose(fptr);
            break;
        case '4':
            if (access(SIGN_DEVICE, F_OK))
            {
                printf("Error! file doesnt exist\n");
                exit(1);
            }
            else if ((fptr = fopen(SIGN_DEVICE, "w")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            /* write 48 byte hash to the device file */
            command = SIGN_COMMAND;
            msg_size = SIGN_MSG_SIZE;
            resp_size = SIGN_RESP_SIZE;
            send_offset = SIGN_OFFSET;
            response_offset = SIGN_MSG_SIZE + SIGN_OFFSET;
            buff = inbuff + 3; /* + 3 to skip response code */
            fprintf(fptr, "%c%c%c%c%c%c%c%c%c47f05d367b0c32e438fb63e6cf4a5f35c2aa2f90dc7543f8",
                    command,
                    (char)msg_size, (char)(msg_size >> 8),
                    (char)resp_size, (char)(resp_size >> 8),
                    (char)send_offset, (char)(send_offset >> 8),
                    (char)response_offset, (char)(response_offset >> 8)
                    );
            fclose(fptr);
            if ((fptr = fopen(SIGN_DEVICE, "r")) == NULL)
            {
                printf("Error! opening signature device file\n");
                exit(1);
            }
            /* read back 104 byte DER format signature */
            fread(inbuff, SIGN_READ_BYTES, 1, fptr);
            fclose(fptr);
            printf("status signature:\r\n%.*s\r\n", SIGN_READ_BYTES, inbuff);
            break;
        case '5':
            if (access(DIGEST_DEVICE, F_OK))
            {
                printf("Error! file doesnt exist\n");
                exit(1);
            }
            else if ((fptr = fopen(DIGEST_DEVICE, "w")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            command = DIGEST_COMMAND;
            msg_size = DIGEST_MSG_SIZE;
            resp_size = DIGEST_RESP_SIZE;
            send_offset = DIGEST_OFFSET;
            response_offset = DIGEST_OFFSET;
            fprintf(fptr, "%c%c%c%c%c%c%c%c%c",
                    command,
                    (char)msg_size, (char)(msg_size >> 8),
                    (char)resp_size, (char)(resp_size >> 8),
                    (char)send_offset, (char)(send_offset >> 8),
                    (char)response_offset, (char)(response_offset >> 8));
            fclose(fptr);
            if ((fptr = fopen(DIGEST_DEVICE, "r")) == NULL)
            {
                printf("Error! opening file\n");
                exit(1);
            }
            fseek(fptr, DIGEST_SERIAL_OFFSET, SEEK_SET);
            fscanf(fptr, "%[^\n]", inbuff);
            printf("Icicle kit serial number: %s\n", inbuff);
            fclose(fptr);
            break;
        case 'e':
            return 0;

        default:
            break;
        }
    }
    return 0;
}