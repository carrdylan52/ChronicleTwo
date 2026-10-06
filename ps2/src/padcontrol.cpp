#include "common.h"
#include "padcontrol.hpp"
#include "gamepad.hpp"

// Code (.text)
void CPadControl::Initialize() {
    for (int i = 0; i < PAD_CTRL_BTN_MAX; i++) {
        btn[i].config = 0;
    }
    for (int i = 0; i < PAD_CTRL_ANALOG_MAX; i++) {
        analog[i].axis = PAD_CTRL_AXIS_NONE;
    }
}

int CPadControl::RegisterBtn(int no, int button, int trigger) {
    if (no < 0 || no >= PAD_CTRL_BTN_MAX) {
        return 0;
    }
    btn[no].value = 0;
    btn[no].config = trigger | button;
    return 1;
}

int CPadControl::RegisterAnalog(int no, int axis) {
    if (no < 0 || no >= PAD_CTRL_ANALOG_MAX) {
        return 0;
    }
    analog[no].axis = axis;
    analog[no].value = 0.0f;
    return 1;
}

int CPadControl::Btn(int no) {
    if (no < 0 || no >= PAD_CTRL_BTN_MAX) {
        return 0;
    }
    return btn[no].value;
}

float CPadControl::Analog(int no) {
    if (no < 0 || no >= PAD_CTRL_ANALOG_MAX) {
        return 0.0f;
    }
    return analog[no].value;
}

void CPadControl::Update(CGamePad *pad) {
    rx = pad->GetRXf();
    ry = pad->GetRYf();
    lx = pad->GetLXf();
    ly = pad->GetLYf();

    for (int i = 0; i < PAD_CTRL_BTN_MAX; i++) {
        PAD_CTRL_BTN *entry = &btn[i];
        int config = btn[i].config;
        if (config != 0) {
            int button = config & PAD_CTRL_BUTTON_MASK;
            switch (config & PAD_CTRL_TRIGGER_MASK) {
            case PAD_CTRL_TRIGGER_ON:
                entry->value = pad->On(button);
                break;
            case PAD_CTRL_TRIGGER_DOWN:
                entry->value = pad->Down(button);
                break;
            case PAD_CTRL_TRIGGER_UP:
                entry->value = pad->Up(button);
                break;
            }
        }
    }

    for (int i = 0; i < PAD_CTRL_ANALOG_MAX; i++) {
        PAD_CTRL_ANALOG *entry = &analog[i];
        switch (analog[i].axis) {
        case PAD_CTRL_AXIS_LX:
            entry->value = lx;
            break;
        case PAD_CTRL_AXIS_LY:
            entry->value = ly;
            break;
        case PAD_CTRL_AXIS_RX:
            entry->value = rx;
            break;
        case PAD_CTRL_AXIS_RY:
            entry->value = ry;
            break;
        case PAD_CTRL_AXIS_NONE:
            break;
        }
    }
}
