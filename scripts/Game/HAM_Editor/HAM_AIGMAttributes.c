static HAM_AISettingsComponent HAM_AISettingsFromItem(Managed item)
{
    BaseGameMode gamemode = GetGame().GetGameMode();
    if (!gamemode || item != gamemode)
        return null;

    return HAM_AISettingsComponent.Cast(gamemode.FindComponent(HAM_AISettingsComponent));
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIPerceptionFactor : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetPerceptionFactor());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetPerceptionFactor(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIThreatenedPerception : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetThreatenedPerceptionMul());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetThreatenedPerceptionMul(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAISuppressedAudibleDistance : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetSuppressedAudibleDistance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetSuppressedAudibleDistance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIAimErrorScale : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetAimErrorScale());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetAimErrorScale(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIReactionScale : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetReactionScale());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetReactionScale(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIEngageDelayScale : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetEngageDelayScale());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetEngageDelayScale(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAICoverSeekOverride : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetCoverSeekOverride());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int mode = value;
        settings.HAM_GMSetCoverSeekOverride(mode);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAISkill : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetAISkill());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float skillValue;
        if (!ConvertIndexToValue(var.GetInt(), skillValue))
            return;

        int skill = skillValue;
        settings.HAM_GMSetAISkill(skill);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIEngagementDistance : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetMaxEngagementDistance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetMaxEngagementDistance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIFireAtWillDistance : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetFireAtWillDistance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetFireAtWillDistance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIReturnFireDistance : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetReturnFireDistance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetReturnFireDistance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIContactShareRange : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetContactShareRange());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetContactShareRange(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIFinishOffChance : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetFinishOffChance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetFinishOffChance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIReconDrones : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetReconDrones() && HAM_DroneSupport.IsAvailable());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetReconDrones(var.GetBool());
    }

    override void UpdateInterlinkedVariables(SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, bool isInit = false)
    {
        bool available = HAM_DroneSupport.IsAvailable();

        if (isInit)
        {
            manager.SetAttributeAsSubAttribute(HAM_GMAttributeAIBombingDrones);
            manager.SetAttributeAsSubAttribute(HAM_GMAttributeAIReconDronesMax);
            manager.SetAttributeAsSubAttribute(HAM_GMAttributeAIReconDronesInterval);
            manager.SetAttributeAsSubAttribute(HAM_GMAttributeAIReconDronesAlertGroups);
        }

        bool active = available && var && var.GetBool();
        manager.SetAttributeEnabled(HAM_GMAttributeAIReconDrones, available);
        manager.SetAttributeEnabled(HAM_GMAttributeAIBombingDrones, active);
        manager.SetAttributeEnabled(HAM_GMAttributeAIReconDronesMax, active);
        manager.SetAttributeEnabled(HAM_GMAttributeAIReconDronesInterval, active);
        manager.SetAttributeEnabled(HAM_GMAttributeAIReconDronesAlertGroups, active);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIReconDronesAlertGroups : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetReconDronesAlertGroups());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetReconDronesAlertGroups(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIReconDronesMax : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetReconDronesMax());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetReconDronesMax(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIReconDronesInterval : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetReconDronesInterval());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetReconDronesInterval(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIBombingDrones : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetBombingDrones() && HAM_DroneSupport.IsAvailable());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetBombingDrones(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAISpareUnarmed : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetSpareUnarmed());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetSpareUnarmed(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIConsumeMagazines : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetConsumeMagazines());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetConsumeMagazines(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAICombatStyle : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetCombatStyle());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float styleValue;
        if (!ConvertIndexToValue(var.GetInt(), styleValue))
            return;

        int style = styleValue;
        settings.HAM_GMSetCombatStyle(style);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAILook : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetLookMode());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int mode = value;
        settings.HAM_GMSetLookMode(mode);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIWeapon : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetWeaponMode());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        settings.HAM_GMSetWeaponMode(value);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIFireMode : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetFireMode());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int mode = value;
        settings.HAM_GMSetFireMode(mode);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIMoveFormation : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetMoveFormation());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int formation = value;
        settings.HAM_GMSetMoveFormation(formation);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIAttentionDistance : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetAttentionDistance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetAttentionDistance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIContactMemory : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetContactMemory());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetContactMemory(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIStance : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetStanceMode());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int mode = value;
        settings.HAM_GMSetStanceMode(mode);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIFootNavPreset : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetFootNavPreset());
        if (index == -1)
        {
            Print(string.Format("[HAM_AI] Attribut navigation à pied : valeur stockée inconnue (%1), affichage du défaut",
                settings.HAM_GetFootNavPreset()), LogLevel.WARNING);
            index = ConvertValueToIndex(HAM_EAIFootNavPreset.BALANCED);
            if (index == -1) return null;
        }

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int preset = value;
        settings.HAM_GMSetFootNavPreset(preset);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIVehicleCruiseSpeed : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetVehicleCruiseSpeed());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetVehicleCruiseSpeed(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIStayInVehicle : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetStayInVehicleMode());
        if (index == -1) return null;

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int mode = value;
        settings.HAM_GMSetStayInVehicleMode(mode);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIFuelConsumption : SCR_BaseEditorAttribute
{
    override void UpdateInterlinkedVariables(SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, bool isInit = false)
    {
        if (!manager)
            return;

        if (isInit)
        {
            manager.SetAttributeAsSubAttribute(HAM_GMAttributeFuelConsumptionScale);
        }

        manager.SetAttributeEnabled(HAM_GMAttributeFuelConsumptionScale, !var || var.GetBool());
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetFuelConsumption());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetFuelConsumption(var.GetBool(), playerID);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeFuelConsumptionScale : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetFuelConsumptionScale());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        settings.HAM_GMSetFuelConsumptionScale(var.GetFloat(), playerID);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAIVehicleNavPreset : SCR_BaseFloatValueHolderEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;

        int index = ConvertValueToIndex(settings.HAM_GetVehicleNavPreset());
        if (index == -1)
        {
            Print(string.Format("[HAM_AI] Attribut navigation véhicule : valeur stockée inconnue (%1), affichage du défaut",
                settings.HAM_GetVehicleNavPreset()), LogLevel.WARNING);
            index = ConvertValueToIndex(HAM_EAIVehicleNavPreset.ROADS_PREFERRED);
            if (index == -1) return null;
        }

        return SCR_BaseEditorAttributeVar.CreateInt(index);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;

        float value;
        if (!ConvertIndexToValue(var.GetInt(), value))
            return;

        int preset = value;
        settings.HAM_GMSetVehicleNavPreset(preset);
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAINVGAuto : SCR_BaseEditorAttribute
{
    override void UpdateInterlinkedVariables(SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, bool isInit = false)
    {
        if (!manager)
            return;

        if (isInit)
        {
            manager.SetAttributeAsSubAttribute(HAM_GMAttributeAINVGPerception);
        }
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetNVGAuto());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetNVGAuto(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAINightPerception : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetNightPerceptionMul());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetNightPerceptionMul(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeAINVGPerception : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref SCR_EditorAttributeBaseValues m_BaseValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_BaseValues)
            outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetNVGPerceptionMul());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_AISettingsComponent settings = HAM_AISettingsFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetNVGPerceptionMul(var.GetFloat());
    }
}
