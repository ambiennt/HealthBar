#pragma once

#include <hook.h>
#include <base/base.h>
#include <base/log.h>
#include <base/ezplayer.h>
#include <yaml.h>
#include <base/scheduler.h>
#include <base/playerdb.h>
#include <Actor/ServerPlayer.h>
#include <Actor/Attribute.h>
#include <Core/DataItem.h>
#include <Core/BuildPlatform.h>
#include <Level/Dimension.h>
#include <Packet/SetActorDataPacket.h>
#include <Actor/SynchedActorData.h>

inline struct Settings {

	bool useResourcePackGlyphs = false;

	template <typename IO> static inline bool io(IO f, Settings &settings, YAML::Node &node) {
		return f(settings.useResourcePackGlyphs, node["useResourcePackGlyphs"]);
	}
} settings;

namespace HealthBarUtils {

inline constexpr const char* HEALTH_GLYPH      = "\ue1fe"; // , glyph 0xE1FE
inline constexpr const char* ABSORPTION_GLYPH  = "\ue1ff"; // , glyph 0xE1FF
inline constexpr const char* HEALTH_ASCII      = "\u00a7\u0063\u2764\u00a7\u0072"; // §c❤§r
inline constexpr const char* ABSORPTION_ASCII  = "\u00a7\u0065\u2764\u00a7\u0072"; // §e❤§r

std::string buildPlatformToString(BuildPlatform b);
std::string getHealthBarNameTag(Player &player);
SetActorDataPacket getHealthBarPacket(Player &player);
void broadcastHealthBar(Player &player, Player* exceptPlayer);
void syncAllHealthBarData(Player &joiningPlayer);

} // namespace HealthBarUtils

DEF_LOGGER("HealthBar");