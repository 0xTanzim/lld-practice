#include <iostream>
using namespace std;

class Command {
public:
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual ~Command() = default;
};

class Light {
public:
    void on() { cout << "Light is ON" << endl; }
    void off() { cout << "Light is OFF" << endl; }
};

class Fan {
public:
    void on() { cout << "Fan is ON" << endl; }
    void off() { cout << "Fan is OFF" << endl; }
};

class LightCommand : public Command {
private:
    Light *light;

public:
    explicit LightCommand(Light *l) : light(l) {}

    void execute() override { light->on(); }
    void undo() override { light->off(); }
};

class FanCommand : public Command {
private:
    Fan *fan;

public:
    explicit FanCommand(Fan *f) : fan(f) {}

    void execute() override { fan->on(); }
    void undo() override { fan->off(); }
};

class RemoteController {
public:
    static constexpr int NUM_BUTTONS = 4;

private:
    Command *buttons[NUM_BUTTONS] = {};
    bool buttonOn[NUM_BUTTONS] = {};

    static bool isValidIndex(int idx) {
        return idx >= 0 && idx < NUM_BUTTONS;
    }

public:
    ~RemoteController() {
        for (int i = 0; i < NUM_BUTTONS; ++i) {
            delete buttons[i];
        }
    }

    void setCommand(int idx, Command *cmd) {
        if (!isValidIndex(idx)) {
            cerr << "Invalid button index: " << idx << endl;
            return;
        }
        delete buttons[idx];
        buttons[idx] = cmd;
        buttonOn[idx] = false;
    }

    void pressButton(int idx) {
        if (!isValidIndex(idx) || buttons[idx] == nullptr) {
            cout << "No command assigned at button " << idx << endl;
            return;
        }
        if (buttonOn[idx]) {
            buttons[idx]->undo();
        } else {
            buttons[idx]->execute();
        }
        buttonOn[idx] = !buttonOn[idx];
    }
};

int main() {
    Light livingRoomLight;
    Fan ceilingFan;
    RemoteController remote;

    remote.setCommand(0, new LightCommand(&livingRoomLight));
    remote.setCommand(1, new FanCommand(&ceilingFan));

    cout << "--- Toggling Light Button 0 ---" << endl;
    remote.pressButton(0);
    remote.pressButton(0);

    cout << "--- Toggling Fan Button 1 ---" << endl;
    remote.pressButton(1);
    remote.pressButton(1);

    cout << "--- Pressing Unassigned Button 2 ---" << endl;
    remote.pressButton(2);

    remote.setCommand(99, new LightCommand(&livingRoomLight));

    return 0;
}
