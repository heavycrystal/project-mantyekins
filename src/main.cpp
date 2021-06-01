#include "project_mantyekins.h"

int main(int argc, char** argv)
{
    std::ifstream file(argv[argc - 1], std::ios::in | std::ios::ate | std::ios::binary);
    int size = file.tellg();
    uint8_t file_data[size];
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(file_data), size);

    cartridge_header header = cartridge_header(file_data);
    std::cout << "Was parse successful? " << header.was_parse_successful() << "\n";
    std::cout << "Is ROM bootable? " << header.is_cartridge_bootable() << "\n";
    std::cout << "Does Cartridge have RTC? " << header.is_timer_present() << "\n";    
    std::cout << "Does Cartridge have RAM? " << header.is_ram_present() << "\n";
    std::cout << "Does Cartridge have battery? " << header.is_battery_present() << "\n";

    std::cout << "MBC Type: " << header.get_mbc_type() << "\n";
    std::cout << "ROM Size: " << header.get_rom_size() << "\n";
    std::cout << "RAM Size: " << header.get_ram_size() << "\n";
    return 0;
}
