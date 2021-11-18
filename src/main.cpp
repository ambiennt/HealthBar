#include <dllentry.h>
#include "main.h"

DEFAULT_SETTINGS(settings);

static std::unordered_map<Player*, std::pair<float, float>> attributes;

constexpr const char* inputModeToString(BuildPlatform b) {
    switch (b) {
        case BuildPlatform::UWP:
        case BuildPlatform::Win32:
            return "PC";

        case BuildPlatform::Android:
        case BuildPlatform::iOS:
        case BuildPlatform::Amazon:
        case BuildPlatform::WindowsPhone:
            return "Mobile";

        case BuildPlatform::Xbox:
        case BuildPlatform::PS4:
        case BuildPlatform::Nintendo:
            return "Controller";

        default: return "Unknown";
    }
}

void UpdateHealthBar(Player *player, Player *initalizedPlayer, float currHealth, float currAbsorption) {

    //player->mInputMode always returns 1
    auto nametag = player->mPlayerName + " §7[" + inputModeToString(player->mBuildPlatform) + "]§r\n" + std::to_string((int) currHealth);
    if (settings.useResourcePackGlyphs) {
        nametag += "" + (currAbsorption > 0.0f ? " " + std::to_string((int) currAbsorption) + "" : ""); // glyph 0xE1FE, 0xE1FF
    }
    else {
        nametag += "§c❤§r" + (currAbsorption > 0.0f ? " " + std::to_string((int) currAbsorption) + "§e❤§r" : "");
    }

    SetActorDataPacket pkt;
    pkt.rid = player->mRuntimeID;
    pkt.items.emplace_back(std::make_unique<DataItem2<std::string>>(ActorDataIDs::NAMETAG, nametag));

    if (initalizedPlayer) {
        initalizedPlayer->sendNetworkPacket(pkt);
    }
    else {
        player->mDimension->forEachPlayer([&](Player &p) -> bool {
            p.sendNetworkPacket(pkt);
            return true;
        });
    }
}

THook(void, "?normalTick@Player@@UEAAXXZ", Player* player) {
    original(player);

    auto &[prevHealth, prevAbsorption] = attributes[player];
    float currHealth = getAttribute(player, 7)->currentVal;
    float currAbsorption = getAttribute(player, 14)->currentVal;

    bool shouldUpdate = currHealth != prevHealth || currAbsorption != prevAbsorption;
    if (shouldUpdate) {
        UpdateHealthBar(player, nullptr, currHealth, currAbsorption);
    }
    prevHealth = currHealth;
    prevAbsorption = currAbsorption;
}

THook(void*, "??0AddPlayerPacket@@QEAA@AEAVPlayer@@@Z", void* pkt, Player *player) {
    auto ret = original(pkt, player);
    // yes I know this is a terrible way to update the health bar when the player comes back into view but
    // appending the nametag to the addplayer packet keeps crashing and I dont know how to fix it
    Mod::Scheduler::SetTimeOut(Mod::Scheduler::GameTick(1), [=](auto) {
        float currHealth = getAttribute(player, 7)->currentVal;
        float currAbsorption = getAttribute(player, 14)->currentVal;
        UpdateHealthBar(player, nullptr, currHealth, currAbsorption);
    });
    return ret;
}

void dllenter() {}
void dllexit() {}
void PreInit() {
    Mod::PlayerDatabase::GetInstance().AddListener(SIG("initialized"), [](Mod::PlayerEntry const &entry) {
        entry.player->mDimension->forEachPlayer([&](Player &p) -> bool {
            float currHealth = getAttribute(&p, 7)->currentVal;
            float currAbsorption = getAttribute(&p, 14)->currentVal;
            UpdateHealthBar(&p, entry.player, currHealth, currAbsorption);
            return true;
        });
    });

    Mod::PlayerDatabase::GetInstance().AddListener(SIG("left"), [](Mod::PlayerEntry const &entry) {
        attributes.erase(entry.player);
    });
}
void PostInit() {}