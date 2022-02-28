#define L_RS PB10
#define L_RW PB11
#define L_E PB12
#define L_D4 PB5
#define L_D5 PB6
#define L_D6 PB7
#define L_D7 PB8
//PA5 SCK
//PA6 MOSI
//PA7 MISO
//

#define ISO15693_POLYCRC16 0x8408
#define ISO15693_MASKCRC16 0x0001
#define ISO15693_PRELOADCRC16 0xFFFF


class D{
public:
    constexpr static uint8_t D100[4] = {0x00,0x00,0x00,0x64};
    constexpr static uint8_t D500[4] = {0x00,0x00,0x01,0xf4};
    constexpr static uint8_t D1000[4] = {0x00,0x00,0x03,0xe8};
};