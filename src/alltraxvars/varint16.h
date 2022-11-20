#pragma once
#include "var.h"

class VarInt16 : protected Var
{
public:
    VarInt16(const char* name, uint32_t addr, short minVal, short maxVal, double convertVal, int machineOffset, const char* unitLabel)
        : Var(name, VarType::INT16, 1, addr, (long)minVal, (long)maxVal, convertVal, machineOffset, unitLabel){}

    // Getters
    const char* getUnitLabel() { return Var::getUnitLabel(); }
    short getMinVal() { return Var::getMinVal(); }
    short getMaxVal() { return Var::getMaxVal(); }
    short getVal() {
        if(this->_array != nullptr)
            return this->_array[0];
        return 0;
    }
    short* getArray() {

        // Get the inherited value array, return if null
        long* value = Var::getValue();
        if(value == nullptr)
            return nullptr;
        
        // Map the values of the value array to a new short array
        short* array = new short[Var::getArrayLen()];
        for(int i = 0; i < Var::getArrayLen(); i++)
            array[i] = (short)value[i];
        return array;
    }

    // Setters
    void setMinVal(short val) { Var::setMinVal(val); }
    void setMaxVal(short val) { Var::setMaxVal(val); }
    void setVal(short val) {
        if(_array != nullptr)
            delete _array;
        _array = new short[1];
        _array[0] = val;
    }
    void setArray(short* array, int length) {
        long* newArray = new long[length];
        for(int i = 0; i < length; i++)
            newArray[i] = (long)array[i];
        Var::setValue(newArray, length);
    }

    // Functions
    double convertToReal() { return Var::convertToReal((long)getVal()); }
    double convertToReal(short value) { return Var::convertToReal((long)value); }
    short convertToMachine(double value) { return (short)Var::convertToMachine(value); }

private:
    short* _array;
};