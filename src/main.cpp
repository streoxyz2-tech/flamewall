#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

#include <algorithm>
#include <deque>

using namespace geode::prelude;

namespace {
    struct BufferedInput {
        int ticksLeft;
        bool down;
        int button;
        bool player1;
    };

    std::deque<BufferedInput> s_inputs;
    bool s_injecting = false;
    bool s_runtimeEnabled = true;

    int getBufferTicks() {
        auto value = Mod::get()->getSettingValue<int64_t>("buffer-ticks");
        return static_cast<int>(std::clamp<int64_t>(value, 2, 16));
    }

    bool shouldBuffer(GJBaseGameLayer* layer, int button) {
        auto* playLayer = PlayLayer::get();
        if (!layer || !playLayer || button != 1 || !s_runtimeEnabled)
            return false;

        if (!Mod::get()->getSettingValue<bool>("enabled"))
            return false;

        // Avoid carrying inputs across death, pause, or level transitions.
        if (playLayer->m_isPaused || playLayer->m_playerDied)
            return false;

        return true;
    }

    void clearBuffer() {
        s_inputs.clear();
    }

    void applyInput(GJBaseGameLayer* layer, BufferedInput const& input) {
        if (!layer)
            return;

        s_injecting = true;
        layer->handleButton(input.down, input.button, input.player1);
        s_injecting = false;
    }

    void advanceBuffer(GJBaseGameLayer* layer) {
        if (!layer || s_inputs.empty())
            return;

        // Preserve event order: a release cannot overtake its matching press.
        for (auto& input : s_inputs) {
            if (input.ticksLeft > 0)
                --input.ticksLeft;
        }

        while (!s_inputs.empty() && s_inputs.front().ticksLeft <= 0) {
            auto input = s_inputs.front();
            s_inputs.pop_front();
            applyInput(layer, input);
        }
    }
}

$on_mod(Loaded) {
    listenForSettingChanges<bool>("enabled", [](bool value) {
        s_runtimeEnabled = true;
        if (!value)
            clearBuffer();
    });
}

$on_game(Loaded) {
    listenForKeybindSettingPresses("toggle-key", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat && PlayLayer::get()) {
            s_runtimeEnabled = !s_runtimeEnabled;
            if (!s_runtimeEnabled)
                clearBuffer();
            Notification::create(
                fmt::format("Input Buffer {}", s_runtimeEnabled ? "ON" : "OFF"),
                NotificationIcon::Info
            )->show();
        }
    });
}

class $modify(PredictiveBufferBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (s_injecting || !shouldBuffer(this, button)) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        // Queue both press and release, so the user's hold duration is preserved.
        // Button 1 is the standard jump input. Other buttons pass through untouched.
        if (s_inputs.size() >= 128) {
            // Fail open rather than dropping inputs if something unusual floods the queue.
            clearBuffer();
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        int delay = getBufferTicks();
        if (!s_inputs.empty())
            delay = std::max(delay, s_inputs.back().ticksLeft);

        s_inputs.push_back({ delay, down, button, isPlayer1 });
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        // Count full physics ticks, not rendered frames or half-tick callbacks.
        if (!isHalfTick)
            advanceBuffer(this);
    }
};

class $modify(PredictiveBufferPlayLayer, PlayLayer) {
    void resetLevel() {
        clearBuffer();
        PlayLayer::resetLevel();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        clearBuffer();
        PlayLayer::destroyPlayer(player, object);
    }
};
