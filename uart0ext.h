#ifndef UART0EXT_H_
#define UART0EXT_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARS 15
#define MAX_FIELDS 5

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

void getsUart0(USER_DATA *data);
void parseFields(USER_DATA *data);
char* getFieldString(USER_DATA *data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber);
int32_t getFieldHex(USER_DATA *data, uint8_t fieldNumber);
bool stringsEqual(const char str1[], const char str2[]);
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);
void clearField(USER_DATA* data);
char* IntToString(uint32_t num, char str[]);
char* HexToString(uint32_t num, char str[]);
void CopyStrings(char str1[], char str2[]);

#endif
