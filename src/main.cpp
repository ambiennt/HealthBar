#include <dllentry.h>
#include "main.h"

DEFAULT_SETTINGS(settings);

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

void UpdateHealthBar(Player *player, Player *initalizedPlayer, int32_t currHealth, int32_t currAbsorption) {

	//player->mInputMode always returns 1 if server authoritative movement is off
	std::string nametag = player->mPlayerName + " §7[" + inputModeToString(player->mBuildPlatform) + "]§r\n" + std::to_string(currHealth);
	if (settings.useResourcePackGlyphs) {
		nametag += "" + ((currAbsorption > 0) ? " " + std::to_string(currAbsorption) + "" : ""); // glyph 0xE1FE, 0xE1FF
	}
	else {
		nametag += "§c❤§r" + ((currAbsorption > 0) ? " " + std::to_string(currAbsorption) + "§e❤§r" : "");
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

TInstanceHook(void, "?normalTick@ServerPlayer@@UEAAXXZ", ServerPlayer) {
	original(this);

	int32_t currHealth = this->getHealthAsInt();
	int32_t currAbsorption = this->getAbsorptionAsInt();

	bool shouldUpdate = ((currHealth != this->EZPlayerFields->mHealthOld) || (currAbsorption != this->EZPlayerFields->mAbsorptionOld));
	if (shouldUpdate) {
		UpdateHealthBar(this, nullptr, currHealth, currAbsorption);
	}
	this->EZPlayerFields->mHealthOld = currHealth;
	this->EZPlayerFields->mAbsorptionOld = currAbsorption;
}

THook(void*, "??0AddPlayerPacket@@QEAA@AEAVPlayer@@@Z", void* pkt, Player &player) {
	auto ret = original(pkt, player);
	// yes I know this is a terrible way to update the health bar when the player comes back into view but
	// appending the nametag to the addplayer packet keeps crashing and I dont know how to fix it
	Mod::Scheduler::SetTimeOut(Mod::Scheduler::GameTick(1), [&](auto) {
		UpdateHealthBar(&player, nullptr, player.getHealthAsInt(), player.getAbsorptionAsInt());
	});
	return ret;
}

void dllenter() {}
void dllexit() {}
void PreInit() {
	Mod::PlayerDatabase::GetInstance().AddListener(SIG("initialized"), [](Mod::PlayerEntry const &entry) {
		entry.player->mDimension->forEachPlayer([&](Player &p) -> bool {
			UpdateHealthBar(&p, entry.player, entry.player->getHealthAsInt(), entry.player->getAbsorptionAsInt());
			return true;
		});
	});
}
void PostInit() {}