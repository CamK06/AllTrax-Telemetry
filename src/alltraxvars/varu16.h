#pragma once
#include "var.h"

class VarU16 : public Var
{
public:
    VarU16(const char* name, uint32_t addr, uint16_t minVal, uint16_t maxVal, double convertVal, int machineOffset, const char* unitLabel)
        : Var(name, VarType::UINT16, 1, addr, (long)minVal, (long)maxVal, convertVal, machineOffset, unitLabel){}

    // Getters
    const char* getUnitLabel() { return Var::getUnitLabel(); }
    uint16_t getMinVal() { return Var::getMinVal(); }
    uint16_t getMaxVal() { return Var::getMaxVal(); }
    uint16_t getVal() {
        if(this->_array != nullptr)
            return this->_array[0];
        return 0;
    }
    uint16_t* getArray() {

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new short array
        uint16_t* array = new uint16_t[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++)
            array[i] = (uint16_t)value[i];
        return array;
    }

    // Setters
    void setMinVal(uint16_t val) { Var::setMinVal(val); }
    void setMaxVal(uint16_t val) { Var::setMaxVal(val); }
    void setVal(uint16_t val) {
        if(_array != nullptr)
            delete _array;
        _array = new uint16_t[1];
        _array[0] = val;
    }
    void setArray(uint16_t* array, int length) {
        long* newArray = new long[length];
        for(int i = 0; i < length; i++)
            newArray[i] = (long)array[i];
        Var::setValue(newArray, length);
    }

    // Functions
    double convertToReal() { return Var::convertToReal((long)getVal()); }
    double convertToReal(uint16_t value) { return Var::convertToReal((long)value); }
    uint16_t convertToMachine(double value) { return (uint16_t)Var::convertToMachine(value); }

private:
    uint16_t* _array;
};