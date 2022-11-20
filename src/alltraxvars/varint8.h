#pragma once
#include "var.h"

class VarInt8 : protected Var
{
public:
    VarInt8(const char* name, uint32_t addr, int8_t minVal, int8_t maxVal, double convertVal, int machineOffset, const char* unitLabel)
        : Var(name, VarType::SBYTE, 1, addr, (long)minVal, (long)maxVal, convertVal, machineOffset, unitLabel){}

    // Getters
    const char* getUnitLabel() { return Var::getUnitLabel(); }
    int8_t getMinVal() { return Var::getMinVal(); }
    int8_t getMaxVal() { return Var::getMaxVal(); }
    int8_t getVal() {
        if(this->_array != nullptr)
            return this->_array[0];
        return 0;
    }
    int8_t* getArray() {

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new int8_t array
        int8_t* array = new int8_t[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++)
            array[i] = (int8_t)value[i];
        return array;
    }

    // Setters
    void setMinVal(int8_t val) { Var::setMinVal(val); }
    void setMaxVal(int8_t val) { Var::setMaxVal(val); }
    void setVal(int8_t val) {
        if(_array != nullptr)
            delete _array;
        _array = new int8_t[1];
        _array[0] = val;
    }
    void setArray(int8_t* array, int length) {
        long* newArray = new long[length];
        for(int i = 0; i < length; i++)
            newArray[i] = (long)array[i];
        Var::setValue(newArray, length);
    }

    // Functions
    double convertToReal() { return Var::convertToReal((long)getVal()); }
    double convertToReal(int8_t value) { return Var::convertToReal((long)value); }
    int8_t convertToMachine(double value) { return (int8_t)Var::convertToMachine(value); }

private:
    int8_t* _array;
};