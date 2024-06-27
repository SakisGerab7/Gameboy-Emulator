#pragma once

#include "Common.hpp"

class Cartrige {
public:
    Cartrige(const std::string &filename);
    ~Cartrige();

    u8 Read(u16 addr) const;
    void Write(u16 addr, u8 val);

private:
    bool IsMbc1() const;

    bool HasBattery() const;
    void LoadBattery();
    void SaveBattery();

    u8 ReadMbc1(u16 addr) const;
    void WriteMbc1(u16 addr, u8 val);

    const char *GetLicencee() const;
    const char *GetType() const;

private:
    std::string m_Filename;
    std::vector<u8> m_Rom;

    struct CartHeader {
        u8 Entry[4];
        u8 Logo[48];
        u8 Title[16];
        u16 NewLicCode;
        u8 SGBFlag;
        u8 Type;
        u8 RomSize;
        u8 RamSize;
        u8 DestCode;
        u8 OldLicCode;
        u8 Version;
        u8 Checksum;
        u16 GlobalChecksum;
    };

    CartHeader *m_Header;

    // MBC1
    std::vector<u8> m_Ram;
    u8 m_RomBankNumber;
    u8 m_RamBankNumber;
    bool m_RamEnable;
    bool m_RomBankMode;
};