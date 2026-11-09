/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "Dummy.h"

class NOTSA_EXPORT_VTABLE CDummyObject : public CDummy {
public:
    CDummyObject();
    explicit CDummyObject(CObject* obj);

public:
    static void InjectHooks();

    CObject* CreateObject();
    void     UpdateFromObject(CObject* obj);

private:
    // Wrappers for the hooks of the constructors
    CDummyObject* Constructor0() { this->CDummyObject::CDummyObject(); return this; }
    CDummyObject* Constructor1(CObject* obj) { this->CDummyObject::CDummyObject(obj); return this; }
};

VALIDATE_SIZE(CDummyObject, 0x38);
