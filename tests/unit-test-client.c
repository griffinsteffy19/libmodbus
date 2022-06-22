/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <modbus.h>
#include <errno.h>
#include <modbus-tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int  test_iomodule(void);
int  test_scanner_v1(void);
int  test_scanner_v2(void);
int  system_test(void);
void empty_uint16_t_reg(uint16_t * reg, int length);
int  green_spot(modbus_t * mb, int * step);
int  red_spot(modbus_t * mb, int * step);
int  scan_code(modbus_t * mb, modbus_mapping_t * mb_mapping, int * step,
               uint16_t * tab_reg);
int  convert_code(uint16_t * tab_reg, int length);
void print_array_uint16_t(uint16_t * array, int length);
void print_array_uint8_t(uint8_t * array, int length);

#define IOBOX_IP     "192.168.10.2"
#define IOBOX_PORT   502
#define SCANNER_IP   "192.168.10.100"
#define SCANNER_PORT 502
#define CODE_LENGTH  24
char ACCEPTED_CODE[CODE_LENGTH + 1] = "530a81f10be198732902bcc0";
char code[CODE_LENGTH + 1]          = {'\0'};

// #define IOMODULE_TESTs
int main(int argc, char * argv[])
{
    int testnum = atoi(argv[1]);
    int success = 1;
    if(1 == testnum)
    {
        test_iomodule();
    }
    else if(2 == testnum)
    {
        test_scanner_v1();
    }
    else if(3 == testnum)
    {
        test_scanner_v2();
    }
    else if(4 == testnum)
    {
        system_test();
    }
    else
    {
        printf("No Test Specified....\n");
        printf("1 - Test IO Module\n");
        printf("2 - Test DL Scanner\n");
        printf("2 - Test DL Scanner (Auto)\n");
        return -1;
    }
    printf("--- test completed! ---\n");

    return (success) ? 0 : -1;
}

void print_array_uint16_t(uint16_t * array, int length)
{
    for(int i = 0; i < length; i++)
    {
        printf("<%.4X>", array[i]);
    }
    printf("\n");
}

void print_array_uint8_t(uint8_t * array, int length)
{
    for(int i = 0; i < length; i++)
    {
        printf("<%.2X>", array[i]);
    }
    printf("\n");
}

int test_iomodule()
{
    // const int slave = MODBUS_TCP_SLAVE;
    modbus_t * mb;
    int        step = 0;
    int        rt   = 0;
    // uint32_t old_response_to_sec;
    // uint32_t old_response_to_usec;

    printf("--- IO Module Test ---\n");
    mb = modbus_new_tcp("192.168.1.2", 502);
    if(NULL == mb)
    {
        printf("~Failed to Create new TCP Connection\n");
        return -1;
    }
    modbus_set_debug(mb, TRUE);
    modbus_set_response_timeout(mb, 5, 0);
    modbus_set_error_recovery(mb, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);

    step++;
    printf("%0d - Modbus Connect\n", step);
    rt = modbus_connect(mb);
    if(rt == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        goto close;
    }

    // modbus_get_response_timeout(ctx, &old_response_to_sec,
    // &old_response_to_usec); modbus_set_response_timeout(ctx, 0, 600000);

    for(int on_num = 4; on_num <= 7; on_num += 1)
    {
        step++;
        printf("%0d - Register %d on\n", step, on_num);
        rt = modbus_write_bit(mb, on_num, 0xFF00);
        if(-1 == rt)
        {
            goto close;
        }
        sleep(1);
    }

    sleep(2);
    for(int off_num = 7; off_num >= 4; off_num -= 1)
    {
        step++;
        printf("%0d - Register %d off\n", step, off_num);
        rt = modbus_write_bit(mb, off_num, 0x0000);
        if(-1 == rt)
        {
            goto close;
        }
        sleep(1);
    }

close:
    step++;
    printf("%0d - modbus close/free\n", step);
    modbus_close(mb);
    modbus_free(mb);

    step++;
    printf("%0d - Sleep 1s\n", step);
    sleep(1);
    return rt;
}

int test_scanner_v1()
{
    modbus_t * mb;
    int        step = 0;
    int        rt   = 0;

#define TAB_REG_LEN 24
    uint16_t tab_reg[TAB_REG_LEN] = {0};

    printf("--- Scanner Test v1 ---\n");
    step++;
    printf("%0d - Modbus New TCP Connection\n", step);
    mb = modbus_new_tcp(SCANNER_IP, SCANNER_PORT);

    (step)++;
    printf("%0d - Create Modbus Mapping\n", step);
    modbus_mapping_t * mb_mapping =
        modbus_mapping_new_start_address(0, 8, 0, 8, 0, 0, 0, 0);
    if(NULL == mb_mapping)
    {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(mb);
        return -1;
    }

    if(NULL == mb)
    {
        printf("~Failed to Create new TCP Connection\n");
        return -1;
    }
    modbus_set_debug(mb, FALSE);
    modbus_set_response_timeout(mb, 5, 0);
    modbus_set_error_recovery(mb, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);

    step++;
    printf("%0d - Modbus Connect\n", step);
    rt = modbus_connect(mb);
    if(rt == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        goto close;
    }

#define TOTAL_SCAN_ATTEMPTS 1
    int scan_attempts = TOTAL_SCAN_ATTEMPTS;
    for(; scan_attempts > 0; scan_attempts--)
    {
        printf("========== | SCAN ATTEMPT %03d | ==========\n",
               (TOTAL_SCAN_ATTEMPTS - scan_attempts + 1));
        rt = scan_code(mb, mb_mapping, &step, tab_reg);
        if(-1 == rt)
        {
            goto close;
        }
    }

    step++;
    printf("%0d - Flash Red Spot on\n", step);
    // red spot on
    rt = modbus_write_bit(mb, 2, 1);
    if(-1 == rt)
    {
        goto close;
    }
    // red spot off
    rt = modbus_write_bit(mb, 2, 0);
    if(-1 == rt)
    {
        goto close;
    }

close:
    step++;
    printf("%0d - modbus close/free\n", step);
    modbus_close(mb);
    modbus_free(mb);
    modbus_mapping_free(mb_mapping);
    return rt;
}

int scan_code(modbus_t * mb, modbus_mapping_t * mb_mapping, int * step,
              uint16_t * tab_reg)
{
    int rt = 0;
#define OUTPUT_BIT_SIZE 8

    (*step)++;
    printf("%0d - Phase On\n", *step);
    rt = modbus_write_bit(mb, 0, 1);
    if(-1 == rt)
    {
        return rt;
    }

    (*step)++;
    printf("%0d - Sleep 2s\n", *step);

    int     timeout       = 2 * 1000 * 1000; // 2s
    int     read_delay    = 100 * 1000;      // 100ms
    uint8_t input_bits[8] = {0xFF};
    int     addr          = 0;
    int     nb            = 8;
    while(timeout > 0)
    {
        rt = modbus_read_input_bits(mb, addr, nb, input_bits);
        if(-1 == rt)
        {
            return rt;
        }
        if(0x01 == input_bits[7])
        {
            (*step)++;
            printf("%0d - Read %d Input bit(s), starting at %d\n", *step, addr, nb);
            timeout = -1;
        }

        usleep(read_delay);
        timeout -= (read_delay);
    }

    if((-1 - read_delay) != timeout)
    {
        (*step)++;
        printf("%0d - Read %d Input bit(s), starting at %d\n", *step, addr, nb);
    }
    (*step)++;
    printf("%0d - Input bit(s): ", *step);
    print_array_uint8_t(input_bits, 8);

    (*step)++;
    printf("%0d - Flash Green Spot on\n", *step);
    // green spot on
    rt = modbus_write_bit(mb, 1, 1);
    if(-1 == rt)
    {
        return rt;
    }
    // green spot off
    rt = modbus_write_bit(mb, 1, 0);
    if(-1 == rt)
    {
        return rt;
    }

    (*step)++;
    printf("%0d - Sleep 1s\n", *step);
    sleep(1);

    (*step)++;
    printf("%0d - Read %d Input bit(s), starting at %d\n", *step, addr, nb);
    rt = modbus_read_input_bits(mb, addr, nb, input_bits);
    if(-1 == rt)
    {
        return rt;
    }

    (*step)++;
    printf("%0d - Input bit(s): ", *step);
    print_array_uint8_t(input_bits, 8);

    (*step)++;
    int start_reg = 0;
    int num_reg   = 14;
    printf("%0d - Read %d Registers, starting at %d\n", *step, num_reg,
           start_reg);
    rt = modbus_read_registers(mb, start_reg, num_reg, tab_reg);
    if(-1 == rt)
    {
        return rt;
    }

    (*step)++;
    printf("%0d - Convert Scanned Code\n", *step);
    rt = convert_code(tab_reg, num_reg);
    if(-1 == rt)
    {
        return rt;
    }
    printf("Scanned Code: %s\n", code);

    (*step)++;
    printf("%0d - Phase Off\n", *step);
    rt = modbus_write_bit(mb, 0, 0);
    if(-1 == rt)
    {
        return rt;
    }

    return rt;
}

int convert_code(uint16_t * tab_reg, int length)
{
    uint8_t char_value      = 0xFF;
    uint8_t next_char_value = 0xFF;
    int     char_num        = 0;

    for(int reg_num = 0; reg_num < length; reg_num++)
    {
        char_value      = tab_reg[reg_num] >> 8;
        next_char_value = ((uint8_t)(tab_reg[reg_num] & 0xFF));
        if(0 == reg_num && 0x02 != char_value)
        {
            printf("CODE ERROR: No Start Text Character\n");
            // return -1;
            return 0;
        }
        else if(0 < reg_num)
        {
            if(0x0D == char_value || 0x0A == char_value)
            {
                return 0;
            }
            code[char_num] = char_value;
            char_num++;
        }

        if(0x0D == next_char_value || 0x0A == next_char_value)
        {
            return 0;
        }
        code[char_num] = next_char_value;
        char_num++;
    }
    return 0;
}

int test_scanner_v2()
{
    modbus_t * mb;
    int        step = 0;
    int        rt   = 0;

#define TAB_REG_LEN 24
    uint16_t tab_reg[TAB_REG_LEN] = {0};

    printf("--- Scanner Test v2 ---\n");
    // setup
    {
        step++;
        printf("%0d - Modbus New TCP Connection\n", step);
        mb = modbus_new_tcp(SCANNER_IP, SCANNER_PORT);

        if(NULL == mb)
        {
            printf("~Failed to Create new TCP Connection\n");
            return -1;
        }
        modbus_set_debug(mb, FALSE);
        modbus_set_response_timeout(mb, 5, 0);
        modbus_set_error_recovery(mb, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);

        step++;
        printf("%0d - Modbus Connect\n", step);
        rt = modbus_connect(mb);
        if(rt == -1)
        {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            goto close;
        }
    }

    int timeout    = 30 * 1000 * 1000; // us (20s)
    int read_delay = 100 * 1000;       // us (100ms)

    int     addr          = 0;
    int     nb            = 8;
    int     start_reg     = 0;
    int     num_reg       = 14;
    uint8_t input_bits[8] = {0xFF};
    step++;
    printf("%0d - Starting %ds to scan\n", step, (timeout / 1000000));
    while(timeout > 0)
    {
        rt = modbus_read_input_bits(mb, addr, nb, input_bits);
        if(1 == input_bits[7])
        {
            (step)++;
            printf("%0d - Code Detected... Read %d Registers, starting at %d\n", step,
                   num_reg, start_reg);
            rt = modbus_read_registers(mb, start_reg, num_reg, tab_reg);
            if(-1 == rt)
            {
                return rt;
            }
            rt = convert_code(tab_reg, num_reg);
            if(-1 == rt)
            {
                return rt;
            }
            printf("Scanned Code: %s\n", code);
            if(0 == strcmp((char *)code, ACCEPTED_CODE))
            {
                (step)++;
                printf("%0d - Code Accepted\n", step);
                printf("\tScanned: %s\n", code);
                printf("\t  Valid: %s\n", ACCEPTED_CODE);
                green_spot(mb, &step);
                usleep(100 * 1000);
            }
            else
            {
                red_spot(mb, &step);
                usleep(100 * 1000);
            }
            empty_uint16_t_reg(tab_reg, TAB_REG_LEN);
            code[0]       = '\0';
            input_bits[7] = 0xFF;
        }

        if(-1 == rt)
        {
            goto close;
        }
        usleep(read_delay); // ms * us/ms
        timeout -= (read_delay);
    }

close:
    step++;
    printf("%0d - modbus close/free\n", step);
    modbus_close(mb);
    modbus_free(mb);
    return rt;
}

int system_test(void)
{
    modbus_t * mb_scanner;
    modbus_t * mb_iobox;
    int        step = 0;
    int        rt   = 0;

#define TAB_REG_LEN 24
    uint16_t tab_reg[TAB_REG_LEN] = {0};

    printf("--- System Test ---\n");
    // setup
    {
        step++;
        printf("%0d - Modbus New TCP Connection\n", step);
        mb_scanner = modbus_new_tcp(SCANNER_IP, SCANNER_PORT);
        mb_iobox   = modbus_new_tcp(IOBOX_IP, IOBOX_PORT);

        if(NULL == mb_scanner)
        {
            printf("~Failed to Create new TCP Connection [Scanner]\n");
            return -1;
        }
        if(NULL == mb_iobox)
        {
            printf("~Failed to Create new TCP Connection [IO Box]\n");
            return -1;
        }

        modbus_set_debug(mb_scanner, FALSE);
        modbus_set_response_timeout(mb_scanner, 5, 0);
        modbus_set_error_recovery(mb_scanner, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
        modbus_set_debug(mb_iobox, FALSE);
        modbus_set_response_timeout(mb_iobox, 5, 0);
        modbus_set_error_recovery(mb_iobox, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);

        step++;
        printf("%0d - Modbus Connect Scanner\n", step);
        rt = modbus_connect(mb_scanner);
        if(rt == -1)
        {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            goto close;
        }
        step++;
        printf("%0d - Modbus Connect IO Box\n", step);
        rt = modbus_connect(mb_iobox);
        if(rt == -1)
        {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            goto close;
        }
    }

    int timeout    = 30 * 1000 * 1000; // us (20s)
    int read_delay = 100 * 1000;       // us (100ms)

    int     addr          = 0;
    int     nb            = 8;
    int     start_reg     = 0;
    int     num_reg       = 14;
    uint8_t input_bits[8] = {0xFF};
    step++;
    printf("%0d - Starting %ds to scan\n", step, (timeout / 1000000));
    while(timeout > 0)
    {
        rt = modbus_read_input_bits(mb_scanner, addr, nb, input_bits);
        if(1 == input_bits[7])
        {
            (step)++;
            printf("%0d - Code Detected... Read %d Registers, starting at %d\n", step,
                   num_reg, start_reg);
            rt = modbus_read_registers(mb_scanner, start_reg, num_reg, tab_reg);
            if(-1 == rt)
            {
                return rt;
            }
            rt = convert_code(tab_reg, num_reg);
            if(-1 == rt)
            {
                return rt;
            }
            printf("Scanned Code: %s\n", code);
            if(0 == strcmp((char *)code, ACCEPTED_CODE))
            {
                (step)++;
                printf("%0d - Code Accepted\n", step);
                printf("\tScanned: %s\n", code);
                printf("\t  Valid: %s\n", ACCEPTED_CODE);
                green_spot(mb_scanner, &step);

                modbus_write_bit(mb_iobox, 4, 0xFF00);
                usleep(200 * 1000);
                modbus_write_bit(mb_iobox, 4, 0x0000);
            }
            else
            {
                red_spot(mb_scanner, &step);
                usleep(100 * 1000);
            }
            empty_uint16_t_reg(tab_reg, TAB_REG_LEN);
            code[0]       = '\0';
            input_bits[7] = 0xFF;
        }

        if(-1 == rt)
        {
            goto close;
        }
        usleep(read_delay); // ms * us/ms
        timeout -= (read_delay);
    }

close:
    step++;
    printf("%0d - modbus close/free\n", step);
    modbus_close(mb_scanner);
    modbus_free(mb_scanner);
    return rt;
}

void empty_uint16_t_reg(uint16_t * reg, int length)
{
    for(int i = 0; i < length; i++)
    {
        reg[i] = '\0';
    }
    return;
}

int green_spot(modbus_t * mb, int * step)
{
    int rt = 0;
    (*step)++;
    printf("%0d - Flash Green Spot on\n", *step);
    // green spot on
    rt = modbus_write_bit(mb, 1, 1);
    if(-1 == rt)
    {
        return rt;
    }
    // green spot off
    rt = modbus_write_bit(mb, 1, 0);
    // if (-1 == rt) { return rt; }

    return rt;
}

int red_spot(modbus_t * mb, int * step)
{
    int rt = 0;
    (*step)++;
    printf("%0d - Flash Red Spot on\n", *step);
    // green spot on
    rt = modbus_write_bit(mb, 2, 1);
    if(-1 == rt)
    {
        return rt;
    }
    // green spot off
    rt = modbus_write_bit(mb, 2, 0);
    // if (-1 == rt) { return rt; }

    return rt;
}
