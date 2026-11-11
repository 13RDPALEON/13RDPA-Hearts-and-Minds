// ====================================================================================
// HAM_MilsimGMAttributes.c
//
// 6 checkboxes GM « 13RDPA » — fenêtre des réglages du scénario (catégorie
// globale HAM_Milsim.conf, m_bIsGlobalAttributeCategory 1).
// Lisent/écrivent HAM_MilsimSettingsComponent sur le gamemode (autorité serveur,
// réplication RplProp vers les clients).
//
// Pattern éprouvé du projet (cf. HAM_VON_GMAttributes, HAM_MedicalGMAttributes).
// Enregistrés dans Configs/Editor/AttributeLists/Edit.conf (override empilé du GUID
// vanilla {F3D6C6D25642352C}, syntaxe +m_aAttributes).
// ====================================================================================

// ------------------------------------------------------------------------------------
// HELPER — item GM (attribut global = le gamemode) → notre composant de réglages
// ------------------------------------------------------------------------------------
static HAM_MilsimSettingsComponent HAM_MilsimFromItem(Managed item)
{
    BaseGameMode gamemode = GetGame().GetGameMode();
    if (!gamemode || item != gamemode)
        return null;

    return HAM_MilsimSettingsComponent.Cast(gamemode.FindComponent(HAM_MilsimSettingsComponent));
}

// ====================================================================================
// DROPDOWN — « Preset de réglages » (tête de catégorie)
// Pattern HAM_GMAttributeBasePreset À L'IDENTIQUE (prouvé en prod) : les entrées du
// menu (m_aValues, nom + description + valeur) sont définies STATIQUEMENT dans
// Edit.conf ; la valeur (m_fEntryFloatValue) = INDEX du preset dans le registre
// HAM_MilsimPresets.conf que le composant applique côté serveur.
// ⚠️ Garder Edit.conf (libellés/valeurs) et HAM_MilsimPresets.conf (ordre des
// fichiers) ALIGNÉS.
// Choisir un preset applique d'un coup les 20 réglages ; ils restent modifiables
// individuellement ensuite (l'index affiché reste le dernier preset choisi).
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimPreset : SCR_BaseEditorAttribute
{
    [Attribute()]
    protected ref array<ref SCR_EditorAttributeFloatStringValueHolder> m_aValues;

    override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
    {
        if (m_aValues && !m_aValues.IsEmpty())
            outEntries.Insert(new SCR_BaseEditorAttributeFloatStringValues(m_aValues));
        return outEntries.Count();
    }

    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        if (!m_aValues || m_aValues.IsEmpty()) return null;

        // var = INDEX de l'entrée du menu — on retrouve celle dont la valeur
        // correspond au preset courant (pattern HAM_GMAttributeBasePreset)
        float current = settings.HAM_GetPresetIndex();
        for (int i = 0; i < m_aValues.Count(); i++)
        {
            if (m_aValues[i] && Math.AbsFloat(m_aValues[i].GetFloatValue() - current) < 0.5)
                return SCR_BaseEditorAttributeVar.CreateFloat(i);
        }
        return SCR_BaseEditorAttributeVar.CreateFloat(0);
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        if (!m_aValues || m_aValues.IsEmpty()) return;

        int idx = Math.Round(var.GetFloat());
        if (idx < 0 || idx >= m_aValues.Count()) return;
        if (!m_aValues[idx]) return;

        settings.HAM_GMSetPreset(Math.Round(m_aValues[idx].GetFloatValue()));
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Masquer les nametags »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimHideNameTags : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetHideNameTags());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetHideNameTags(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Masquer l'icône inconscient »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimHideUnconsciousIcon : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetHideUnconsciousIcon());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetHideUnconsciousIcon(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Masquer les noms VON »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimHideVONNames : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetHideVONNames());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetHideVONNames(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Conserver les épaves »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimKeepWrecks : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetKeepWrecks());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetKeepWrecks(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Conserver les corps »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimKeepCorpses : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetKeepCorpses());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetKeepCorpses(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimVehicleVirtualization : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetVehicleVirtualization());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetVehicleVirtualization(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Interdire de parler inconscient »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimBlockUnconsciousVON : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetBlockUnconsciousVON());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetBlockUnconsciousVON(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Masquer l'UI VON de proximité ennemie »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimHideEnemyProxVONUI : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetHideEnemyProxVONUI());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetHideEnemyProxVONUI(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Masquer l'UI VON de proximité entière »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimHideAllProxVONUI : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetHideAllProxVONUI());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetHideAllProxVONUI(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Bâtiments indestructibles »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimIndestructibleBuildings : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetIndestructibleBuildings());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetIndestructibleBuildings(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Désactiver Arma Vision (joueurs) »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimBlockPhotoMode : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetBlockPhotoMode());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetBlockPhotoMode(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Désactiver les marqueurs vanilla »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimDisableVanillaMarkers : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetDisableVanillaMarkers());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetDisableVanillaMarkers(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : Supprimer le rang minimum »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimNoMinimumRank : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetNoMinimumRank());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetNoMinimumRank(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : 1re personne forcée à pied »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimForceFirstPersonFoot : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetForceFirstPersonFoot());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetForceFirstPersonFoot(var.GetBool());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : 1re personne forcée en véhicule »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimForceFirstPersonVehicle : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetForceFirstPersonVehicle());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetForceFirstPersonVehicle(var.GetBool());
    }
}

// ====================================================================================
// SLIDER — « 13RDPA : Distance des nametags » (0 = distances d'origine)
// Layout : AttributePrefab_Slider.layout — pattern HAM_GMAttributeVoiceDetectionRadius
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimNameTagMaxDistance : SCR_BaseEditorAttribute
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
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetNameTagMaxDistance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetNameTagMaxDistance(var.GetFloat());
    }
}

// ====================================================================================
// CHECKBOX — « 13RDPA : GM furtif »
// ====================================================================================
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimStealthGM : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetStealthGM());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetStealthGM(var.GetBool());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimWeaponDropChance : SCR_BaseEditorAttribute
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
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateFloat(settings.HAM_GetWeaponDropChance());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetWeaponDropChance(var.GetFloat());
    }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMAttributeMilsimShowCompass : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return null;
        return SCR_BaseEditorAttributeVar.CreateBool(settings.HAM_GetShowCompass());
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var,
        SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        if (!var) return;
        HAM_MilsimSettingsComponent settings = HAM_MilsimFromItem(item);
        if (!settings) return;
        settings.HAM_GMSetShowCompass(var.GetBool());
    }
}
