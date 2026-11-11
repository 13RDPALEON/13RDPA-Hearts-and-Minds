
enum HAM_EAIStanceMode
{
    FROM_GLOBAL,
    FORCE_STAND,
    FORCE_CROUCH,
    FORCE_PRONE,
    NO_PRONE,
    ALERT_UPRIGHT,
    AUTOMATIC
}

class HAM_AIStanceResolver
{
    static bool Resolve(int mode, ECharacterStance desired, bool threatened, out ECharacterStance outStance)
    {
        switch (mode)
        {
            case HAM_EAIStanceMode.FORCE_STAND:
            {
                outStance = ECharacterStance.STAND;
                return true;
            }
            case HAM_EAIStanceMode.FORCE_CROUCH:
            {
                outStance = ECharacterStance.CROUCH;
                return true;
            }
            case HAM_EAIStanceMode.FORCE_PRONE:
            {
                outStance = ECharacterStance.PRONE;
                return true;
            }
            case HAM_EAIStanceMode.NO_PRONE:
            {
                outStance = desired;
                if (desired == ECharacterStance.PRONE)
                    outStance = ECharacterStance.CROUCH;
                return true;
            }
            case HAM_EAIStanceMode.ALERT_UPRIGHT:
            {
                outStance = ECharacterStance.STAND;
                if (threatened)
                    outStance = ECharacterStance.CROUCH;
                return true;
            }
        }

        return false;
    }
}

enum HAM_EAILookMode
{
    FROM_GLOBAL,
    HOLD,
    AUTOMATIC,
    STEADY,
    FOCUSED
}

class HAM_AILookRules
{
    static bool IsSteadyOrMore(int mode)
    {
        return mode == HAM_EAILookMode.STEADY
            || mode == HAM_EAILookMode.FOCUSED
            || mode == HAM_EAILookMode.HOLD;
    }

    static bool IsFocusedOrMore(int mode)
    {
        return mode == HAM_EAILookMode.FOCUSED
            || mode == HAM_EAILookMode.HOLD;
    }

    static bool IsHold(int mode)
    {
        return mode == HAM_EAILookMode.HOLD;
    }

    static bool SectorConcernsMe(int flags, int mode)
    {
        if (flags & SCR_EAIThreatSectorFlags.CAUSED_DAMAGE)
            return true;

        if (IsFocusedOrMore(mode))
            return false;

        return (flags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME) != 0;
    }

    static bool SectorTooFar(vector myPos, vector sectorPos)
    {
        float attention = HAM_AISettingsComponent.HAM_AttentionDistance();
        if (attention <= 0)
            return false;

        return vector.Distance(myPos, sectorPos) > attention;
    }

    static string PrefabName(IEntity entity)
    {
        if (!entity)
            return "inconnu";

        EntityPrefabData data = entity.GetPrefabData();
        if (!data)
            return "sans prefab";

        return FilePath.StripPath(data.GetPrefabName().GetPath());
    }

    static SCR_AICombatComponent CombatOf(AIAgent owner)
    {
        if (!owner)
            return null;

        IEntity controlled = owner.GetControlledEntity();
        if (!controlled)
            return null;

        return SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
    }
}

enum HAM_EAIWeaponMode
{
    FROM_GLOBAL,
    AUTOMATIC,
    ENGAGEMENT_ONLY,
    ENGAGEMENT_STICKY,
    FORCE_LOWERED,
    FORCE_RAISED,
    TEMPORARY_ENGAGEMENT,
    ALERT_ONLY
}

enum HAM_EAIWeaponModeSetting
{
    FROM_GLOBAL,
    AUTOMATIC,
    ENGAGEMENT_ONLY,
    ENGAGEMENT_STICKY = 3,
    ALERT_ONLY = 7
}

enum HAM_EAIStayInVehicleMode
{
    FROM_GLOBAL,
    ALL,
    CREW,
    FREE
}

enum HAM_EAIFootNavPreset
{
    ROADS_PREFERRED,
    BALANCED,
    AVOID_ROADS,
    NO_ROADS,
    TRAILS_PREFERRED
}

enum HAM_EAIVehicleNavPreset
{
    ROADS_ONLY,
    ROADS_PREFERRED,
    FREE,
    AVOID_ASPHALT,
    DIRT_PREFERRED,
    OFFROAD_ONLY
}

enum HAM_EAIFootNavChoice
{
    FROM_GLOBAL,
    ROADS_PREFERRED,
    BALANCED,
    AVOID_ROADS,
    NO_ROADS,
    TRAILS_PREFERRED
}

enum HAM_EAIVehicleNavChoice
{
    FROM_GLOBAL,
    ROADS_ONLY,
    ROADS_PREFERRED,
    FREE,
    AVOID_ASPHALT,
    DIRT_PREFERRED,
    OFFROAD_ONLY
}

enum HAM_EAIZoneFireMode
{
    FOLLOW,
    FIRE_AT_WILL,
    RETURN_FIRE_ONLY,
    HOLD_FIRE
}

enum HAM_EAIZoneStyleChoice
{
    FOLLOW,
    DEFAULT,
    OFFENSIVE,
    URBAN,
    DEFENSIVE,
    CQB_DEFENSIVE,
    CQB_OFFENSIVE
}

enum HAM_EAICoverSeekOverride
{
    FROM_STYLE,
    FORCE_FIRE_FIRST,
    FORCE_SEEK_COVER
}

enum HAM_EAIFormationChoice
{
    FROM_STYLE,
    WEDGE,
    LINE,
    COLUMN,
    STAGGERED_COLUMN
}

enum HAM_EAIMoveFormation
{
    FROM_GENERAL,
    WEDGE,
    LINE,
    COLUMN,
    STAGGERED_COLUMN
}

enum HAM_EAICombatRole
{
    NONE,
    ASSAULT,
    COVER
}

enum HAM_EAICoverRequirement
{
    FROM_STYLE,
    REQUIRE,
    IGNORE
}

enum HAM_EAIMagazineConsumption
{
    FROM_GLOBAL,
    CONSUME,
    INFINITE
}

enum HAM_EAICombatStyle
{
    DEFAULT,
    OFFENSIVE,
    URBAN,
    DEFENSIVE,
    MECHANIZED,
    CQB_DEFENSIVE,
    CQB_OFFENSIVE
}

class HAM_AICombatStyle
{
    protected static ref array<ref HAM_AIStylePreset> s_aTRN_PresetCache;
    protected static ref array<bool> s_aTRN_PresetLoaded;

    static HAM_AIStylePreset GetPreset(HAM_EAICombatStyle style)
    {
        int index = style;
        if (index < 0 || index > HAM_EAICombatStyle.CQB_OFFENSIVE)
            return null;

        if (!s_aTRN_PresetCache)
        {
            s_aTRN_PresetCache = {};
            s_aTRN_PresetLoaded = {};
            for (int i = 0; i <= HAM_EAICombatStyle.CQB_OFFENSIVE; i++)
            {
                s_aTRN_PresetCache.Insert(null);
                s_aTRN_PresetLoaded.Insert(false);
            }
        }

        if (s_aTRN_PresetLoaded[index])
            return s_aTRN_PresetCache[index];

        ResourceName path = HAM_AISettingsComponent.HAM_StylePresetPath(index);
        if (path.IsEmpty())
            return null;

        s_aTRN_PresetLoaded[index] = true;

        Resource container = BaseContainerTools.LoadContainer(path);
        if (!container)
        {
            Print(string.Format("[HAM_AI] Preset de style illisible : %1", path), LogLevel.WARNING);
            return null;
        }

        HAM_AIStylePreset preset = HAM_AIStylePreset.Cast(
            BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));

        if (!preset)
        {
            Print(string.Format("[HAM_AI] Preset de style invalide : %1", path), LogLevel.WARNING);
            return null;
        }

        s_aTRN_PresetCache[index] = preset;
        return preset;
    }

    static bool MoveFormationToVanilla(int move, out SCR_EAIGroupFormation formation)
    {
        if (move == HAM_EAIMoveFormation.WEDGE)
            formation = SCR_EAIGroupFormation.Wedge;
        else if (move == HAM_EAIMoveFormation.LINE)
            formation = SCR_EAIGroupFormation.Line;
        else if (move == HAM_EAIMoveFormation.COLUMN)
            formation = SCR_EAIGroupFormation.Column;
        else if (move == HAM_EAIMoveFormation.STAGGERED_COLUMN)
            formation = SCR_EAIGroupFormation.StaggeredColumn;
        else
            return false;

        return true;
    }

    static SCR_EAIGroupFormation ResolveFormation(HAM_EAIFormationChoice choice, HAM_EAICombatStyle style)
    {
        switch (choice)
        {
            case HAM_EAIFormationChoice.WEDGE:             return SCR_EAIGroupFormation.Wedge;
            case HAM_EAIFormationChoice.LINE:              return SCR_EAIGroupFormation.Line;
            case HAM_EAIFormationChoice.COLUMN:            return SCR_EAIGroupFormation.Column;
            case HAM_EAIFormationChoice.STAGGERED_COLUMN:  return SCR_EAIGroupFormation.StaggeredColumn;
        }

        return GetTeamFormation(style);
    }

    static bool ResolveRequiresCover(HAM_EAICoverRequirement choice, HAM_EAICombatStyle style)
    {
        if (choice == HAM_EAICoverRequirement.REQUIRE)
            return true;

        if (choice == HAM_EAICoverRequirement.IGNORE)
            return false;

        return RequiresCover(style);
    }

    static SCR_EAIGroupFormation GetTeamFormation(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_eTeamFormation;

        switch (style)
        {
            case HAM_EAICombatStyle.OFFENSIVE:     return SCR_EAIGroupFormation.Line;
            case HAM_EAICombatStyle.URBAN:         return SCR_EAIGroupFormation.StaggeredColumn;
            case HAM_EAICombatStyle.DEFENSIVE:     return SCR_EAIGroupFormation.Wedge;
            case HAM_EAICombatStyle.MECHANIZED:    return SCR_EAIGroupFormation.Column;
            case HAM_EAICombatStyle.CQB_DEFENSIVE: return SCR_EAIGroupFormation.Wedge;
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return SCR_EAIGroupFormation.Column;
        }
        return SCR_EAIGroupFormation.Wedge;
    }

    static float GetMoveFormationScale(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_fMoveFormationScale;

        switch (style)
        {
            case HAM_EAICombatStyle.OFFENSIVE:     return 2.5;
            case HAM_EAICombatStyle.URBAN:         return 1.8;
            case HAM_EAICombatStyle.DEFENSIVE:     return 2.0;
            case HAM_EAICombatStyle.MECHANIZED:    return 1.5;
            case HAM_EAICombatStyle.CQB_DEFENSIVE: return 1.0;
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return 0.8;
        }
        return 1.0;
    }

    static float GetApproachFloor(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_fApproachFloor;

        switch (style)
        {
            case HAM_EAICombatStyle.OFFENSIVE:     return 8.0;
            case HAM_EAICombatStyle.URBAN:         return 6.0;
            case HAM_EAICombatStyle.DEFENSIVE:     return 40.0;
            case HAM_EAICombatStyle.CQB_DEFENSIVE: return 60.0;
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return 2.0;
        }
        return -1;
    }

    static float GetHoldTimeS(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_fHoldTimeS;

        switch (style)
        {
            case HAM_EAICombatStyle.OFFENSIVE:     return 2.5;
            case HAM_EAICombatStyle.URBAN:         return 4.0;
            case HAM_EAICombatStyle.DEFENSIVE:     return 10.0;
            case HAM_EAICombatStyle.CQB_DEFENSIVE: return 20.0;
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return 1.5;
        }
        return 5.0;
    }

    static int GetLookMode(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_eLookMode;

        switch (style)
        {
            case HAM_EAICombatStyle.OFFENSIVE:     return HAM_EAILookMode.STEADY;
            case HAM_EAICombatStyle.URBAN:         return HAM_EAILookMode.STEADY;
            case HAM_EAICombatStyle.DEFENSIVE:     return HAM_EAILookMode.FOCUSED;
            case HAM_EAICombatStyle.CQB_DEFENSIVE: return HAM_EAILookMode.FOCUSED;
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return HAM_EAILookMode.FOCUSED;
        }

        return HAM_EAILookMode.FROM_GLOBAL;
    }

    static float GetDefendRadius(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return Math.Max(2.0, preset.m_fDefendRadius);

        return 3.0;
    }

    static int FootNavChoiceToPreset(int choice)
    {
        switch (choice)
        {
            case HAM_EAIFootNavChoice.ROADS_PREFERRED:  return HAM_EAIFootNavPreset.ROADS_PREFERRED;
            case HAM_EAIFootNavChoice.BALANCED:         return HAM_EAIFootNavPreset.BALANCED;
            case HAM_EAIFootNavChoice.AVOID_ROADS:      return HAM_EAIFootNavPreset.AVOID_ROADS;
            case HAM_EAIFootNavChoice.NO_ROADS:         return HAM_EAIFootNavPreset.NO_ROADS;
            case HAM_EAIFootNavChoice.TRAILS_PREFERRED: return HAM_EAIFootNavPreset.TRAILS_PREFERRED;
        }
        return -1;
    }

    static int VehicleNavChoiceToPreset(int choice)
    {
        switch (choice)
        {
            case HAM_EAIVehicleNavChoice.ROADS_ONLY:      return HAM_EAIVehicleNavPreset.ROADS_ONLY;
            case HAM_EAIVehicleNavChoice.ROADS_PREFERRED: return HAM_EAIVehicleNavPreset.ROADS_PREFERRED;
            case HAM_EAIVehicleNavChoice.FREE:            return HAM_EAIVehicleNavPreset.FREE;
            case HAM_EAIVehicleNavChoice.AVOID_ASPHALT:   return HAM_EAIVehicleNavPreset.AVOID_ASPHALT;
            case HAM_EAIVehicleNavChoice.DIRT_PREFERRED:  return HAM_EAIVehicleNavPreset.DIRT_PREFERRED;
            case HAM_EAIVehicleNavChoice.OFFROAD_ONLY:    return HAM_EAIVehicleNavPreset.OFFROAD_ONLY;
        }
        return -1;
    }

    static int ZoneStyleChoiceToStyle(int choice)
    {
        switch (choice)
        {
            case HAM_EAIZoneStyleChoice.DEFAULT:       return HAM_EAICombatStyle.DEFAULT;
            case HAM_EAIZoneStyleChoice.OFFENSIVE:     return HAM_EAICombatStyle.OFFENSIVE;
            case HAM_EAIZoneStyleChoice.URBAN:         return HAM_EAICombatStyle.URBAN;
            case HAM_EAIZoneStyleChoice.DEFENSIVE:     return HAM_EAICombatStyle.DEFENSIVE;
            case HAM_EAIZoneStyleChoice.CQB_DEFENSIVE: return HAM_EAICombatStyle.CQB_DEFENSIVE;
            case HAM_EAIZoneStyleChoice.CQB_OFFENSIVE: return HAM_EAICombatStyle.CQB_OFFENSIVE;
        }
        return -1;
    }

    static bool RequiresCover(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_bRequiresCover;

        switch (style)
        {
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return false;
            case HAM_EAICombatStyle.OFFENSIVE:     return false;
        }
        return true;
    }

    static bool FiresBeforeSeekingCover(HAM_EAICombatStyle style)
    {
        HAM_AIStylePreset preset = GetPreset(style);
        if (preset)
            return preset.m_bFiresBeforeSeekingCover;

        switch (style)
        {
            case HAM_EAICombatStyle.OFFENSIVE:     return true;
            case HAM_EAICombatStyle.DEFENSIVE:     return true;
            case HAM_EAICombatStyle.CQB_DEFENSIVE: return true;
            case HAM_EAICombatStyle.CQB_OFFENSIVE: return true;
        }
        return false;
    }

    static bool OverridesProgression(HAM_EAICombatStyle style)
    {
        return style != HAM_EAICombatStyle.DEFAULT && style != HAM_EAICombatStyle.MECHANIZED;
    }
}

enum HAM_EAIFireModeChoice
{
    FROM_GLOBAL,
    AUTOMATIC,
    SEMI,
    BURST,
    FULL_AUTO
}

enum HAM_EAIFireModeSetting
{
    AUTOMATIC = 1,
    SEMI = 2,
    BURST = 3,
    FULL_AUTO = 4
}
