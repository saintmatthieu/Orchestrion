#include "Gamepad.h"
#include "types/number.h"
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <SDL2/SDL.h>
#include <memory>

namespace dgk
{

class GamepadImpl {

public:

    GamepadImpl(Gamepad *parent) : m_parent(parent) {

    }

    void connect() {
        doConnect();
    }
    void disconnect() {
        running.store(false);
    }

private:
    void doConnect() {
        if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }
    
        if (SDL_NumJoysticks() < 1) {
            std::cout << "No game controller detected!\n";
        } else {
            m_controller = SDL_GameControllerOpen(0);
            if (m_controller) {
                std::cout << "Controller connected: " << SDL_GameControllerName(m_controller) << std::endl;
                std::cout << "Has sensor SDL_SENSOR_ACCEL: " << SDL_GameControllerHasSensor(m_controller, SDL_SENSOR_ACCEL) << std::endl;
                std::cout << "Has sensor SDL_SENSOR_GYRO: " << SDL_GameControllerHasSensor(m_controller, SDL_SENSOR_GYRO) << std::endl;
                // TODO: think about motion controls
                // SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_ACCEL, SDL_TRUE);
                // SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_GYRO, SDL_TRUE);
    
            } else {
                std::cerr << "Could not open game controller: " << SDL_GetError() << std::endl;
            }
        }

        SDL_AddEventWatch([](void* userdata, SDL_Event* event) -> int {
            auto self = (GamepadImpl *)userdata;
            return self->eventWatcher(nullptr, event);
        }, this);
    
        std::thread thread([this]() { eventLoop(); });
        thread.detach();
    }

    int eventWatcher(void* userdata, SDL_Event* event) {
        (void) userdata;
        switch (event->type) {
            case SDL_CONTROLLERAXISMOTION:
                std::cout << "Axis " << static_cast<int>(event->caxis.axis)
                          << " moved: " << event->caxis.value << std::endl;
                break;
            case SDL_CONTROLLERBUTTONDOWN: {
                double velocity = (double) SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY) / UINT16_MAX + 0.5;
                m_parent->onButtonPressed(event->cbutton.button, velocity);
                std::cout << "Button " << static_cast<int>(event->cbutton.button)
                          << " pressed" << std::endl;

                break;
            }
            case SDL_CONTROLLERBUTTONUP: {
                double velocity = (double) SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY) / UINT16_MAX + 0.5;
                m_parent->onButtonReleased(event->cbutton.button, velocity);
                std::cout << "Button " << static_cast<int>(event->cbutton.button)
                          << " released" << std::endl;
                break;
            }
            case SDL_CONTROLLERSENSORUPDATE: {
                static auto p = event->csensor.data;
                auto d = event->csensor.data;
                if (!(muse::is_equal(p[0], d[0]) && muse::is_equal(p[1], d[1]) && muse::is_equal(p[2], d[2]) 
                   && muse::is_equal(p[3], d[3]) && muse::is_equal(p[4], d[4]) && muse::is_equal(p[5], d[5]))) {
                    p = event->csensor.data;
                    std::cout << std::fixed << std::setprecision(5);
                    std::cout << "Motion sensor: " 
                        << p[0] << " : " << p[1] << " : " << p[2] << " : " 
                        << p[3] << " : " << p[4] << " : " << p[5] 
                        << std::endl;
                }
                
                break;
            }
            default:
                break;
        }
        return 1;
    }
        
    void eventLoop() {
        while (running) {
            SDL_PumpEvents();
            SDL_Delay(10);
        }
    }

    std::atomic<bool> running {true};
    Gamepad* m_parent = nullptr;
    SDL_GameController* m_controller = nullptr;
};


Gamepad::Gamepad() {
    m_impl = std::make_shared<GamepadImpl>(this);
    m_impl->connect();
}

Gamepad::~Gamepad() {
    m_impl->disconnect();
}

muse::async::Channel<int, double> Gamepad::buttonPressed() const {
    return m_buttonPressed;
}

muse::async::Channel<int, double> Gamepad::buttonReleased() const {
    return m_buttonReleased;
}

void Gamepad::onButtonPressed(int button, double value) {
    m_buttonPressed.send(button, value);
}

void Gamepad::onButtonReleased(int button, double value) {
    m_buttonReleased.send(button, value);
}

}
