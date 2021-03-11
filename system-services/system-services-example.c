// SPDX-License-Identifier: MIT
/*
 * MSS system services example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>

static void display_output(char *data, unsigned int byte_length)
{
    unsigned int inc;
    char byte = 0;
    char buffer[2048];
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
    char c[4096];
    char chr;
    FILE *fptr;

    for (;/*ever*/;)
    {
        printf("PolarFire SoC system services test program.\r\nPress:\r\n");
        printf("1 - to show the serial number\r\n2 - to show the fpga digest\r\n3 - to cat hwrng, until ctrl+c\r\n4 - to request a signature\r\ne - to exit this program\r\n");
        chr = getchar();
        getchar();

        switch (chr)
        {
        case '1':
            if ((fptr = fopen("/dev/mpfs_serial_num", "r")) == NULL)
            {
                printf("Error! opening file");
                exit(1);
            }
            fscanf(fptr, "%[^\n]", c);
            printf("Icicle kit serial number: %s\n", c);
            fclose(fptr);
            break;
        case '2':
            if ((fptr = fopen("/dev/mpfs_fpga_digest", "r")) == NULL)
            {
                printf("Error! opening file");
                exit(1);
            }
            printf("pfsoc fpga digest:\n");
            while (fread(c, 66, 1, fptr) == 1)
            {
                printf("%s", c);
            }
            if (feof(fptr))
            {
                printf("%s\n", c);
            }
            fclose(fptr);
            break;
        case '3':
            if ((fptr = fopen("/dev/hwrng", "r")) == NULL)
            {
                printf("Error! opening file");
                exit(1);
            }
            for (;/*ever*/;)
            {
                fread(c, 32, 1, fptr);
                display_output(c, 32);
            }
            fclose(fptr);
            break;
        case '4':
            if ((fptr = fopen("/dev/mpfs_signature", "w")) == NULL)
            {
                printf("Error! opening file");
                exit(1);
            }
            fprintf(fptr, "47f05d367b0c32e438fb63e6cf4a5f35c2aa2f90dc7543f8");
            fclose(fptr);
            if ((fptr = fopen("/dev/mpfs_signature", "r")) == NULL)
            {
                printf("Error! opening file");
                exit(1);
            }
            fread(c, 195, 1, fptr); //3 for status and a space, 192 chars for the hex signature (96 bytes)
            printf("status signature:\r\n%sq\r\n", c);
            fclose(fptr);
            break;
        case 'd':
            if ((fptr = fopen("/dev/mpfs_fpga_digest", "r")) == NULL)
            {
                printf("Error! opening file");
                exit(1);
            }
            fseek(fptr, 50, SEEK_SET);
            fscanf(fptr, "%[^\n]", c);
            printf("Icicle kit serial number: %s\n", c);
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