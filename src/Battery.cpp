#include "Battery.hpp"

Battery::Battery(Cartridge* c)
: cartridge(c) {}

Battery::~Battery() = default;

void Battery::save() {
    if (!cartridge->mbc->ramBank)
        return;

    std::string savePath = cartridge->fileName + ".sav";
    std::ofstream saveFile(savePath, std::ios::binary);

    if (!saveFile) {
        printf("Battery::save(): Could not open file %s\n", savePath.c_str());
        return;
    }

    if (cartridge->mbc->hasInternalRam)
        saveFile.write(reinterpret_cast<char*>(cartridge->mbc->ramBank), 512);
    else
        for (int i = 0; i < cartridge->mbc->ramBanksCount; i++)
            saveFile.write(reinterpret_cast<char*>(cartridge->mbc->ramBanks[i]), 0x2000);


    saveFile.close();
    std::cout << "Saved to " << savePath << std::endl;
}

void Battery::load() {
    if (!cartridge->mbc->ramBank)
        return;

    std::string savePath = cartridge->fileName + ".sav";
    std::ifstream saveFile(savePath, std::ios::binary);

    if (!saveFile) {
        printf("Battery::load(): Could not open file %s\n", savePath.c_str());
        return;
    }

    if (cartridge->mbc->hasInternalRam)
        saveFile.read(reinterpret_cast<char*>(cartridge->mbc->ramBank), 512);
    else
        for (int i = 0; i < cartridge->mbc->ramBanksCount; i++)
            saveFile.read(reinterpret_cast<char*>(cartridge->mbc->ramBanks[i]), 0x2000);

    saveFile.close();

    std::cout << "Loaded save data from " << savePath << std::endl;
}
