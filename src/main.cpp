#include <dllentry.h>
#include "main.h"

DEFAULT_SETTINGS(settings);

void dllenter() {}
void dllexit() {}
void PreInit() {
	Mod::PlayerDatabase::GetInstance().AddListener(SIG("initialized"), [](const Mod::PlayerEntry &entry) {
		HealthBarUtils::syncAllHealthBarData(*(entry.player));
	});
}
void PostInit() {}

std::string HealthBarUtils::buildPlatformToString(BuildPlatform b) {
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

std::string HealthBarUtils::getHealthBarNameTag(Player &player) {

	int32_t currHealth = player.getHealthAsInt();
	int32_t currAbsorption = player.getAbsorptionAsInt();

	// player.mInputMode always returns 1 if server authoritative movement is off
	// use player.mBuildPlatform instead
	std::string nametag(player.mPlayerName + " §7[" + HealthBarUtils::buildPlatformToString(player.mBuildPlatform) + "]§r\n" + std::to_string(currHealth));

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

// obtains a SetActorDataPacket with the updated health/absorption for the given player
SetActorDataPacket HealthBarUtils::getHealthBarPacket(Player &player) {
	SetActorDataPacket pkt;
	pkt.rid = player.mRuntimeID;
	pkt.items.emplace_back(std::make_unique<DataItem2<std::string>>(
		ActorDataIDs::NAMETAG, HealthBarUtils::getHealthBarNameTag(player)));
	
	return pkt;
}

// sends the new health bar nametag from the player to all players in the dimension (including the player who's health changed)
// use when a player changes health
void HealthBarUtils::broadcastHealthBar(Player &player) {
	auto pkt = HealthBarUtils::getHealthBarPacket(player);
	player.mDimension->forEachPlayer([&pkt](Player &p) -> bool {
		p.sendNetworkPacket(pkt);
		return true;
	});
}

// sends all health bar nametags from all players in the dimension (including the joining player) to the joining player
// used when a new player joins the server
void HealthBarUtils::syncAllHealthBarData(Player &joiningPlayer) {
	joiningPlayer.mDimension->forEachPlayer([&joiningPlayer](Player &p) -> bool {
		auto pkt = HealthBarUtils::getHealthBarPacket(p);
		joiningPlayer.sendNetworkPacket(pkt);
		return true;
	});
}

TInstanceHook(void, "?normalTick@ServerPlayer@@UEAAXXZ", ServerPlayer) {
	original(this);

	int32_t currHealth = this->getHealthAsInt();
	int32_t currAbsorption = this->getAbsorptionAsInt();
	bool shouldUpdate = ((currHealth != this->mEZPlayer->mHealthOld) ||
						(currAbsorption != this->mEZPlayer->mAbsorptionOld));

	if (shouldUpdate) {
		HealthBarUtils::broadcastHealthBar(*this);
	}
}

// this is a hacky way to update the health bar when a player comes into view (not necessarily one that has just joined the server)
// appending the nametag to the AddPlayerPacket keeps crashing and idk how to fix it
THook(void*, "??0AddPlayerPacket@@QEAA@AEAVPlayer@@@Z", void* pkt, Player &newPlayer) {
	auto ret = original(pkt, newPlayer);
	Mod::Scheduler::SetTimeOut(Mod::Scheduler::GameTick(1), [&newPlayer](auto) {
		HealthBarUtils::broadcastHealthBar(newPlayer);
	});
	return ret;
}