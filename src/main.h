#pragma once

#include <hook.h>
#include <base/base.h>
#include <base/log.h>
#include <yaml.h>
#include <base/scheduler.h>
#include <base/playerdb.h>
#include <Actor/Player.h>
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

DEF_LOGGER("HealthBar");