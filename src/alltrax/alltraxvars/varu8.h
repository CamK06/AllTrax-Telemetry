#pragma once
#include "var.h"

class VarU8 : public Var
{
public:
    VarU8(const char* name, uint32_t addr, uint8_t minVal, uint8_t maxVal, double convertVal, int machineOffset, const char* unitLabel)
        : Var(name, VarType::BYTE, 1, addr, (long)minVal, (long)maxVal, convertVal, machineOffset, unitLabel){}

    // Getters
    const char* getUnitLabel() { return Var::getUnitLabel(); }
    uint8_t getMinVal() { return Var::getMinVal(); }
    uint8_t getMaxVal() { return Var::getMaxVal(); }
    uint8_t getVal() {
        if(this->_array != nullptr)
            return this->_array[0];
        return 0;
    }
    uint8_t* getArray() {

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new uint8_t array
        uint8_t* array = new uint8_t[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++)
            array[i] = (uint8_t)value[i];
        return array;
    }

    // Setters
    void setMinVal(uint8_t val) { Var::setMinVal(val); }
    void setMaxVal(uint8_t val) { Var::setMaxVal(val); }
    void setVal(uint8_t val) {
        if(_array != nullptr)
            delete _array;
        _array = new uint8_t[1];
        _array[0] = val;
    }
    void setArray(uint8_t* array, int length) {
        long* newArray = new long[length];
        for(int i = 0; i < length; i++)
            newArray[i] = (long)array[i];
        Var::setValue(newArray, length);
    }

    // Functions
    double convertToReal() { return Var::convertToReal((long)getVal()); }
    double convertToReal(uint8_t value) { return Var::convertToReal((long)value); }
    uint8_t convertToMachine(double value) { return (uint8_t)Var::convertToMachine(value); }

private:
    uint8_t* _array;
};