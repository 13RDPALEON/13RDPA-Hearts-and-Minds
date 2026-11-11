modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{
    protected AIGroup m_HAM_CachedGroup;
    protected SCR_AIGroupUtilityComponent m_HAM_GroupUtility;

    protected override void OnThreatSectorEscalation(SCR_AISectorThreatFilter ts, int sectorId, float dangerValue)
    {
        if (m_Target && m_CombatComponent && m_CombatComponent.IsTargetVisible(m_Target))
            return;

        super.OnThreatSectorEscalation(ts, sectorId, dangerValue);
    }

    override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
    {
        int tree = super.ResolveFireTree(target, visible, weaponReady, fireRate);

        if (tree != FIRE_TREE_BURST && tree != FIRE_TREE_SINGLE && tree != FIRE_TREE_SUPPRESSIVE)
            return tree;

        if (target && HAM_HoldFireBeyondDistance(target))
            return FIRE_TREE_LOOK;

        return tree;
    }

    protected bool HAM_HoldFireBeyondDistance(notnull BaseTarget target)
    {
        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (!groupUtility)
            return false;

        int zoneMode = groupUtility.HAM_GetZoneFireMode();
        if (zoneMode == HAM_EAIZoneFireMode.HOLD_FIRE)
            return true;

        if (zoneMode == HAM_EAIZoneFireMode.FIRE_AT_WILL)
            return false;

        if (zoneMode == HAM_EAIZoneFireMode.RETURN_FIRE_ONLY)
            return !target.IsEndangering();

        float fireDist = groupUtility.HAM_GetFireAtWillDistance();
        if (fireDist <= 0)
            return false;

        float dist = target.GetDistance();
        if (dist <= fireDist)
            return false;

        if (target.IsEndangering())
        {
            float riposteDist = groupUtility.HAM_GetReturnFireDistance();
            if (riposteDist <= 0 || dist <= Math.Max(fireDist, riposteDist))
                return false;
        }

        return true;
    }

    protected SCR_AIGroupUtilityComponent HAM_GetGroupUtility()
    {
        if (!m_UtilityComponent)
            return null;

        AIAgent agent = m_UtilityComponent.GetAIAgent();
        if (!agent)
            return null;

        AIGroup group = agent.GetParentGroup();
        if (group != m_HAM_CachedGroup)
        {
            m_HAM_CachedGroup = group;
            m_HAM_GroupUtility = null;

            if (group)
                m_HAM_GroupUtility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
        }

        return m_HAM_GroupUtility;
    }
}
