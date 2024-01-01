#pragma once
#include "Cartridge.hpp"

#include <iostream>
#include <string>

class Battery {
public:
    Battery(Cartridge* c);
    ~Battery();

public:
    void save(); // Save the battery to disk
    void load(); // Load the battery from disk

private:
    Cartridge* cartridge;
};