#include "Cartrige.hpp"
#include "Log.hpp"

Cartrige::Cartrige(const std::string &filename)
    : m_Filename(filename)
{
    std::ifstream fs(filename, std::ios::binary);
    if (!fs) {
        Log::Error("Could not open %s\n", filename.c_str());
        return;
    }

    fs.seekg(0, std::ios::end);
    usize romSize = fs.tellg();
    fs.seekg(0, std::ios::beg);

    m_Rom = std::vector<u8>(romSize);
    fs.read(reinterpret_cast<char*>(&m_Rom[0]), romSize);
    fs.close();

    m_Header = reinterpret_cast<CartHeader*>(&m_Rom[0x100]);
    m_Header->Title[15] = '\0';

    if (IsMbc1()) {
        m_RamEnable = false;
        m_RomBankMode = true;
        m_RomBankNumber = 1;
        m_RamBankNumber = 0;
        m_Ram = std::vector<u8>(0x2000, 0);
    }

    if (HasBattery()) {
        LoadBattery();
    }

    Log::Info("Cartridge Loaded:\n");
    Log::Info("  Title    : %s\n", m_Header->Title);
    Log::Info("  Type     : %x (%s)\n", m_Header->Type, GetType());
    Log::Info("  ROM Size : %d KB, (Measured %ld bytes)\n", 32 << m_Header->RomSize, m_Rom.size());
    Log::Info("  RAM Size : %x, (Measured %ld bytes)\n", m_Header->RamSize, m_Ram.size());
    Log::Info("  LIC Code : %x, %x (%s)\n", m_Header->OldLicCode, m_Header->NewLicCode, GetLicencee());
    Log::Info("  ROM Vers : %x\n", m_Header->Version);

    u8 checksum = 0;
    for (usize addr = 0x134; addr <= 0x14C; addr++) {
        checksum -= m_Rom[addr] + 1;
    }

    if (m_Header->Checksum != (checksum & 0xFF)) {
        Log::Error("Checksum Test Failed\n");
        return;
    }
}

Cartrige::~Cartrige() {
    if (HasBattery()) {
        SaveBattery();
    }
}

u8 Cartrige::Read(u16 addr) const {
    if (IsMbc1()) {
        return ReadMbc1(addr);
    }

    return m_Rom[addr];
}

void Cartrige::Write(u16 addr, u8 val) {
    if (IsMbc1()) {
        WriteMbc1(addr, val);
        return;
    }

    Log::Error("ROM Only Cartrige. Can't Write (addr 0x%04X)\n", addr);
}

bool Cartrige::IsMbc1() const {
    return (m_Header->Type == 0x01)
        || (m_Header->Type == 0x02)
        || (m_Header->Type == 0x03);
}

bool Cartrige::HasBattery() const {
    return (m_Header->Type == 0x03); // For now
}

void Cartrige::LoadBattery() {
    std::ifstream fs(m_Filename + ".sav", std::ios::binary);
    if (!fs) {
        Log::Error("Could not open %s.sav\n", m_Filename.c_str());
        return;
    }

    fs.read(reinterpret_cast<char*>(&m_Ram[0]), m_Ram.size());
    fs.close();
}

void Cartrige::SaveBattery() {
    std::ofstream fs(m_Filename + ".sav", std::ios::binary);
    if (!fs) {
        Log::Error("Could not open %s.sav\n", m_Filename.c_str());
        return;
    }

    fs.write(reinterpret_cast<char*>(&m_Ram[0]), m_Ram.size());
    fs.close();
}

u8 Cartrige::ReadMbc1(u16 addr) const {
    if (0 <= addr && addr <= 0x3FFF) {
        return m_Rom[addr];
    }

    if (0x4000 <= addr && addr <= 0x7FFF) {
        uint32_t realAddr = addr - 0x4000;
        realAddr += 0x4000 * m_RomBankNumber;
        return m_Rom[realAddr]; 
    }

    if (0xA000 <= addr && addr <= 0xBFFF && m_RamEnable) {
        addr -= 0xA000;
        addr += 0x2000 * m_RamBankNumber;
        return m_Ram[addr];
    }

    return 0;
}

void Cartrige::WriteMbc1(u16 addr, u8 val) {
    if (0 <= addr && addr <= 0x1FFF) {
        m_RamEnable = ((val & 0xF) == 0xA);
    }

    if (0x2000 <= addr && addr <= 0x3FFF) {
        if (val == 0) val = 1;
        m_RomBankNumber = (m_RomBankNumber & 0x60) | (val & 0x1F);
    }

    if (0x4000 <= addr && addr <= 0x5FFF) {
        if (m_RomBankMode) {
            m_RomBankNumber |= ((val & 0x3) << 5);
        } else {
            m_RamBankNumber = val;
        }
    }

    if (0x6000 <= addr && addr <= 0x7FFF) {
        m_RomBankMode = (val == 0);
    }

    if (0xA000 <= addr && addr <= 0xBFFF) {
        addr -= 0xA000;
        addr += 0x2000 * m_RamBankNumber;
        m_Ram[addr] = val;
    }
}

const char *Cartrige::GetLicencee() const {
    if (m_Header->OldLicCode == 0x33) {
        switch (m_Header->NewLicCode) {
            case 0x00: return "None";
            case 0x01: return "Nintendo R&D1";
            case 0x08: return "Capcom";
            case 0x13: return "Electronic Arts";
            case 0x18: return "Hudson Soft";
            case 0x19: return "b-ai";
            case 0x20: return "kss";
            case 0x22: return "pow";
            case 0x24: return "PCM Complete";
            case 0x25: return "san-x";
            case 0x28: return "Kemco Japan";
            case 0x29: return "seta";
            case 0x30: return "Viacom";
            case 0x31: return "Nintendo";
            case 0x32: return "Bandai";
            case 0x33: return "Ocean/Acclaim";
            case 0x34: return "Konami";
            case 0x35: return "Hector";
            case 0x37: return "Taito";
            case 0x38: return "Hudson";
            case 0x39: return "Banpresto";
            case 0x41: return "Ubi Soft";
            case 0x42: return "Atlus";
            case 0x44: return "Malibu";
            case 0x46: return "angel";
            case 0x47: return "Bullet-Proof";
            case 0x49: return "irem";
            case 0x50: return "Absolute";
            case 0x51: return "Acclaim";
            case 0x52: return "Activision";
            case 0x53: return "American sammy";
            case 0x54: return "Konami";
            case 0x55: return "Hi tech entertainment";
            case 0x56: return "LJN";
            case 0x57: return "Matchbox";
            case 0x58: return "Mattel";
            case 0x59: return "Milton Bradley";
            case 0x60: return "Titus";
            case 0x61: return "Virgin";
            case 0x64: return "LucasArts";
            case 0x67: return "Ocean";
            case 0x69: return "Electronic Arts";
            case 0x70: return "Infogrames";
            case 0x71: return "Interplay";
            case 0x72: return "Broderbund";
            case 0x73: return "sculptured";
            case 0x75: return "sci";
            case 0x78: return "THQ";
            case 0x79: return "Accolade";
            case 0x80: return "misawa";
            case 0x83: return "lozc";
            case 0x86: return "Tokuma Shoten Intermedia";
            case 0x87: return "Tsukuda Original";
            case 0x91: return "Chunsoft";
            case 0x92: return "Video system";
            case 0x93: return "Ocean/Acclaim";
            case 0x95: return "Varie";
            case 0x96: return "Yonezawa/s\'pal";
            case 0x97: return "Kaneko";
            case 0x99: return "Pack in soft";
            case 0xA4: return "Konami (Yu-Gi-Oh!)";
            default: return "(UNDEFINED)";
        }
    } else {
        switch (m_Header->OldLicCode) {
            case 0x00: return "None";
            case 0x01: return "Nintendo";
            case 0x08: return "Capcom";
            case 0x09: return "Hot-B";
            case 0x0A: return "Jaleco";
            case 0x0B: return "Coconuts Japan";
            case 0x0C: return "Elite Systems";
            case 0x13: return "EA (Electronic Arts)";
            case 0x18: return "Hudsonsoft";
            case 0x19: return "ITC Entertainment";
            case 0x1A: return "Yanoman";
            case 0x1D: return "Japan Clary";
            case 0x1F: return "Virgin Interactive";
            case 0x24: return "PCM Complete";
            case 0x25: return "San-X";
            case 0x28: return "Kotobuki Systems";
            case 0x29: return "Seta";
            case 0x30: return "Infogrames";
            case 0x31: return "Nintendo";
            case 0x32: return "Bandai";
            case 0x34: return "Konami";
            case 0x35: return "HectorSoft";
            case 0x38: return "Capcom";
            case 0x39: return "Banpresto";
            case 0x3C: return ".Entertainment i";
            case 0x3E: return "Gremlin";
            case 0x41: return "Ubisoft";
            case 0x42: return "Atlus";
            case 0x44: return "Malibu";
            case 0x46: return "Angel";
            case 0x47: return "Spectrum Holoby";
            case 0x49: return "Irem";
            case 0x4A: return "Virgin Interactive";
            case 0x4D: return "Malibu";
            case 0x4F: return "U.S. Gold";
            case 0x50: return "Absolute";
            case 0x51: return "Acclaim";
            case 0x52: return "Activision";
            case 0x53: return "American Sammy";
            case 0x54: return "GameTek";
            case 0x55: return "Park Place";
            case 0x56: return "LJN";
            case 0x57: return "Matchbox";
            case 0x59: return "Milton Bradley";
            case 0x5A: return "Mindscape";
            case 0x5B: return "Romstar";
            case 0x5C: return "Naxat Soft";
            case 0x5D: return "Tradewest";
            case 0x60: return "Titus";
            case 0x61: return "Virgin Interactive";
            case 0x67: return "Ocean Interactive";
            case 0x69: return "EA (Electronic Arts)";
            case 0x6E: return "Elite Systems";
            case 0x6F: return "Electro Brain";
            case 0x70: return "Infogrames";
            case 0x71: return "Interplay";
            case 0x72: return "Broderbund";
            case 0x73: return "Sculptered Soft";
            case 0x75: return "The Sales Curve";
            case 0x78: return "t.hq";
            case 0x79: return "Accolade";
            case 0x7A: return "Triffix Entertainment";
            case 0x7C: return "Microprose";
            case 0x7F: return "Kemco";
            case 0x80: return "Misawa Entertainment";
            case 0x83: return "Lozc";
            case 0x86: return "Tokuma Shoten Intermedia";
            case 0x8B: return "Bullet-Proof Software";
            case 0x8C: return "Vic Tokai";
            case 0x8E: return "Ape";
            case 0x8F: return "I’Max";
            case 0x91: return "Chunsoft Co.";
            case 0x92: return "Video System";
            case 0x93: return "Tsubaraya Productions Co.";
            case 0x95: return "Varie Corporation";
            case 0x96: return "Yonezawa/S’Pal";
            case 0x97: return "Kaneko";
            case 0x99: return "Arc";
            case 0x9A: return "Nihon Bussan";
            case 0x9B: return "Tecmo";
            case 0x9C: return "Imagineer";
            case 0x9D: return "Banpresto";
            case 0x9F: return "Nova";
            case 0xA1: return "Hori Electric";
            case 0xA2: return "Bandai";
            case 0xA4: return "Konami";
            case 0xA6: return "Kawada";
            case 0xA7: return "Takara";
            case 0xA9: return "Technos Japan";
            case 0xAA: return "Broderbund";
            case 0xAC: return "Toei Animation";
            case 0xAD: return "Toho";
            case 0xAF: return "Namco";
            case 0xB0: return "acclaim";
            case 0xB1: return "ASCII or Nexsoft";
            case 0xB2: return "Bandai";
            case 0xB4: return "Square Enix";
            case 0xB6: return "HAL Laboratory";
            case 0xB7: return "SNK";
            case 0xB9: return "Pony Canyon";
            case 0xBA: return "Culture Brain";
            case 0xBB: return "Sunsoft";
            case 0xBD: return "Sony Imagesoft";
            case 0xBF: return "Sammy";
            case 0xC0: return "Taito";
            case 0xC2: return "Kemco";
            case 0xC3: return "Squaresoft";
            case 0xC4: return "Tokuma Shoten Intermedia";
            case 0xC5: return "Data East";
            case 0xC6: return "Tonkinhouse";
            case 0xC8: return "Koei";
            case 0xC9: return "UFL";
            case 0xCA: return "Ultra";
            case 0xCB: return "Vap";
            case 0xCC: return "Use Corporation";
            case 0xCD: return "Meldac";
            case 0xCE: return ".Pony Canyon or";
            case 0xCF: return "Angel";
            case 0xD0: return "Taito";
            case 0xD1: return "Sofel";
            case 0xD2: return "Quest";
            case 0xD3: return "Sigma Enterprises";
            case 0xD4: return "ASK Kodansha Co.";
            case 0xD6: return "Naxat Soft";
            case 0xD7: return "Copya System";
            case 0xD9: return "Banpresto";
            case 0xDA: return "Tomy";
            case 0xDB: return "LJN";
            case 0xDD: return "NCS";
            case 0xDE: return "Human";
            case 0xDF: return "Altron";
            case 0xE0: return "Jaleco";
            case 0xE1: return "Towa Chiki";
            case 0xE2: return "Yutaka";
            case 0xE3: return "Varie";
            case 0xE5: return "Epcoh";
            case 0xE7: return "Athena";
            case 0xE8: return "Asmik ACE Entertainment";
            case 0xE9: return "Natsume";
            case 0xEA: return "King Records";
            case 0xEB: return "Atlus";
            case 0xEC: return "Epic/Sony Records";
            case 0xEE: return "IGS";
            case 0xF0: return "A Wave";
            case 0xF3: return "Extreme Entertainment";
            case 0xFF: return "LJN";
            default: return "(UNDEFINED)";
        }
    }
}

const char *Cartrige::GetType() const {
    switch (m_Header->Type) {
        case 0x00: return "ROM ONLY";
        case 0x01: return "MBC1";
        case 0x02: return "MBC1+RAM";
        case 0x03: return "MBC1+RAM+BATTERY";
        case 0x05: return "MBC2";
        case 0x06: return "MBC2+BATTERY";
        case 0x08: return "ROM+RAM 1";
        case 0x09: return "ROM+RAM+BATTERY 1";
        case 0x0B: return "MMM01";
        case 0x0C: return "MMM01+RAM";
        case 0x0D: return "MMM01+RAM+BATTERY";
        case 0x0F: return "MBC3+TIMER+BATTERY";
        case 0x10: return "MBC3+TIMER+RAM+BATTERY 2";
        case 0x11: return "MBC3";
        case 0x12: return "MBC3+RAM 2";
        case 0x13: return "MBC3+RAM+BATTERY 2";
        case 0x19: return "MBC5";
        case 0x1A: return "MBC5+RAM";
        case 0x1B: return "MBC5+RAM+BATTERY";
        case 0x1C: return "MBC5+RUMBLE";
        case 0x1D: return "MBC5+RUMBLE+RAM";
        case 0x1E: return "MBC5+RUMBLE+RAM+BATTERY";
        case 0x20: return "MBC6";
        case 0x22: return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
        case 0xFC: return "POCKET CAMERA";
        case 0xFD: return "BANDAI TAMA5";
        case 0xFE: return "HuC3";
        case 0xFF: return "HuC1+RAM+BATTERY";
        default: return "(UNDEFINED)";
    }
}