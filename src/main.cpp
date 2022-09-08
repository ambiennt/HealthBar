#include <dllentry.h>
#include "main.h"

DEFAULT_SETTINGS(settings);

void dllenter() {}
void dllexit() {}
void PreInit() {
	Mod::PlayerDatabase::GetInstance().AddListener(SIG("initialized"), [](Mod::PlayerEntry const &entry) {
		entry.player->mDimension->forEachPlayer([&](Player &p) -> bool {
			HealthBarUtils::updateHealthBar(p, entry.player, entry.player->getHealthAsInt(), entry.player->getAbsorptionAsInt());
			return true;
		});
	});
}
void PostInit() {}

std::string HealthBarUtils::inputModeToString(BuildPlatform b) {
	switch (b) {
		case BuildPlatform::UWP:
		case BuildPlatform::Win32:
			return std::string("PC");

		case BuildPlatform::Android:
		case BuildPlatform::iOS:
		case BuildPlatform::Amazon:
		case BuildPlatform::WindowsPhone:
			return std::string("Mobile");

		case BuildPlatform::Xbox:
		case BuildPlatform::PS4:
		case BuildPlatform::Nintendo:
			return std::string("Controller");

		default: return std::string("Unknown");
	}
}

std::string HealthBarUtils::getHealthBarNameTag(Player &player, int32_t currHealth, int32_t currAbsorption) {

	//player->mInputMode always returns 1 if server authoritative movement is off
	std::string nametag = player.mPlayerName + " §7[" + HealthBarUtils::inputModeToString(player.mBuildPlatform) + "]§r\n" + std::to_string(currHealth);

	if (settings.useResourcePackGlyphs) {
		nametag += HEALTH_GLYPH;
		if (currAbsorption > 0) {
			nametag += " " + std::to_string(currAbsorption) + ABSORPTION_GLYPH;
		}
	}
	else {
		nametag += HEALTH_ASCII;
		if (currAbsorption > 0) {
			nametag += " " + std::to_string(currAbsorption) + ABSORPTION_ASCII;
		}
	}
	return nametag;
}

void HealthBarUtils::updateHealthBar(Player &player, Player *initalizedPlayer, int32_t currHealth, int32_t currAbsorption) {

	SetActorDataPacket pkt{};
	pkt.rid = player.mRuntimeID;
	pkt.items.emplace_back(std::make_unique<DataItem2<std::string>>(
		ActorDataIDs::NAMETAG, getHealthBarNameTag(player, currHealth, currAbsorption)));

	if (initalizedPlayer) {
		initalizedPlayer->sendNetworkPacket(pkt);
	}
	else {
		player.mDimension->forEachPlayer([&pkt](const Player &p) -> bool {
			p.sendNetworkPacket(pkt);
			return true;
		});
	}
}

TInstanceHook(void, "?normalTick@ServerPlayer@@UEAAXXZ", ServerPlayer) {
	original(this);

	int32_t currHealth = this->getHealthAsInt();
	int32_t currAbsorption = this->getAbsorptionAsInt();
	bool shouldUpdate = ((currHealth != this->mEZPlayer->mHealthOld) ||
						(currAbsorption != this->mEZPlayer->mAbsorptionOld));

	if (shouldUpdate) {
		HealthBarUtils::updateHealthBar(*this, nullptr, currHealth, currAbsorption);
	}
}

// yes I know this is a terrible way to update the health bar when the player comes back into view but
// appending the nametag to the addplayer packet keeps crashing and I dont know how to fix it
THook(void*, "??0AddPlayerPacket@@QEAA@AEAVPlayer@@@Z", void* pkt, Player &newPlayer) {
	auto ret = original(pkt, newPlayer);
	Mod::Scheduler::SetTimeOut(Mod::Scheduler::GameTick(1), [&newPlayer](auto) {
		HealthBarUtils::updateHealthBar(newPlayer, nullptr, newPlayer.getHealthAsInt(), newPlayer.getAbsorptionAsInt());
	});
	return ret;
}