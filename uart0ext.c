// uart 0 extensions

#include <stdbool.h>
#include <stdint.h>
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "uart0ext.h"

void getsUart0(USER_DATA *data)
{
    uint8_t count = 0, i = 0;
    char c = 0;

    for (i = 0; i < MAX_CHARS; i++)
        data->buffer[i] = '\0';

    while (true)
    {
        c = getcUart0();
        if ((c == 8 || c == 127) && count > 0)
            count--;
        else if (c == 13)
        {
            data->buffer[count] = 0;
            return;
        }
        else if (c == 32 || c >= 32)
        {
            data->buffer[count] = c;
            count++;
            if (count == MAX_CHARS)
            {
                data->buffer[count] = 0;
                return;
            }
        }
    }
}

void parseFields(USER_DATA *data)
{
    uint8_t i = 0;
    while(data->buffer[i] != '\0' && data->fieldCount != MAX_FIELDS)
    {
        if ((data->buffer[i] >= 48 && data->buffer[i] <= 57) || (data->buffer[i] >= 65 && data->buffer[i] <= 90) || (data->buffer[i] >= 97 && data->buffer[i] <= 122))
        {
            data->fieldPosition[data->fieldCount] = i;
            data->fieldType[data->fieldCount] = 'n';

            while ((data->buffer[i] >= 48 && data->buffer[i] <= 57) || (data->buffer[i] >= 65 && data->buffer[i] <= 90) || (data->buffer[i] >= 97 && data->buffer[i] <= 122))
            {
                if ((data->buffer[i] >= 65 && data->buffer[i] <= 90) || (data->buffer[i] >= 97 && data->buffer[i] <= 122))
                    data->fieldType[data->fieldCount] = 'c';
                i++;
            }
            data->fieldCount++;
        }
        i++;
    }
    for (i = 0; i < MAX_CHARS; i++)
    {
        if (!((data->buffer[i] >= 48 && data->buffer[i] <= 57) || (data->buffer[i] >= 65 && data->buffer[i] <= 90) || (data->buffer[i] >= 97 && data->buffer[i] <= 122)))
            data->buffer[i] = '\0';
    }
}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber <= data->fieldCount)
        return &data->buffer[data->fieldPosition[fieldNumber]];
    else
        return '\0';
}

int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber <= data->fieldCount && data->fieldType[fieldNumber] == 'n')
    {
        int8_t len = 0, i = 0;
        uint32_t num = 0, tens = 1;
        while (data->buffer[len + data->fieldPosition[fieldNumber]] != 0)
            len++;
        for (i = len - 1; i >= 0; i--)
        {
            num += (data->buffer[data->fieldPosition[fieldNumber] + i] - 48) * tens;
            tens = tens * 10;
        }
        return num;
    }
    else
        return 0;
}

int32_t getFieldHex(USER_DATA *data, uint8_t fieldNumber)
{
    int8_t len = 0, i = 0;
    uint32_t num = 0;
    char ch = 0;
    while (data->buffer[len + data->fieldPosition[fieldNumber]] != 0)
        len++;
    for (i = 0; i < len; i++)
    {
        ch = data->buffer[data->fieldPosition[fieldNumber] + i];
        if (ch >= '0' && ch <= '9')
            num = num * 16 + (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            num = num * 16 + (ch - 'A' + 10);
    }
    return num;
}

bool stringsEqual(const char str1[], const char str2[])
{
    int8_t i = 0;

    do
    {
        if ((str1[i]) != (str2[i]))
            return false;
        i++;
    }
    while (str1[i] != '\0' || str2[i] != '\0');

    return true;
}

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    bool valid = stringsEqual(strCommand, &data->buffer[data->fieldPosition[0]]);;

    if (valid && minArguments <= data->fieldCount - 1)
        return true;
    else
        return false;
}

void clearField(USER_DATA *data)
{
    uint8_t i = 0;

    for (i = 0; i < MAX_CHARS; i++)
        data->buffer[i] = '\0';

    data->fieldCount = 0;
}

char* IntToString(uint32_t num, char str[])
{
    uint8_t len = 0, i = 0;
    uint64_t mod = 10;

    while ((num % mod) != num)
    {
        mod *= 10;
        len++;
    }

    for (i = 0; i <= len; i++)
        str[i] = (num % mod / (mod /= 10) + 48);

    str[len + 1] = 0;

    return str;
}

char* HexToString(uint32_t num, char str[])
{
    uint8_t len = 0, i = 0;
    uint64_t mod = 16;

    str[0] = '0';
    str[1] = 'x';

    while ((num % mod) != num)
    {
        len++;
        mod *= 16;
    }

    for (i = 2; i <= 9; i++)    // clears old str. temporary forn ow
        str[i] = '0';

    for (i = (9 - len); i <= 9; i++)
    {
        str[i] = num % mod / (mod /= 16);
        if (str[i] > 9)
            str[i] += 55;
        else
            str[i] += 48;
    }

    str[10] = 0;

    return str;
}

void CopyStrings(char str1[], char str2[])
{
    uint16_t i = 0;

    while (str1[i] != '\0')
    {
        str2[i] = str1[i];
        i++;
    }

    str2[i] = '\0';
}
