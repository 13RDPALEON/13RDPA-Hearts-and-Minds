class HAM_AIFormationScaleModifier : AITaskScripted
{
    protected const string PORT_OUT_FORMATION_SCALE = "FormationScaleOut";

    protected AIGroup m_CachedGroup;
    protected SCR_AIGroupUtilityComponent m_GroupUtility;

    override protected void OnInit(AIAgent owner)
    {
        HAM_ResolveGroupUtility(owner);
    }

    protected void HAM_ResolveGroupUtility(AIAgent owner)
    {
        AIGroup group = owner.GetParentGroup();
        if (group == m_CachedGroup)
            return;

        m_CachedGroup = group;
        m_GroupUtility = null;

        if (group)
            m_GroupUtility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        HAM_ResolveGroupUtility(owner);

        float scale = 1.0;
        if (m_GroupUtility)
            scale = m_GroupUtility.HAM_GetFormationScale();

        SetVariableOut(PORT_OUT_FORMATION_SCALE, scale);

        return ENodeResult.SUCCESS;
    }

    protected static ref TStringArray s_aVarsOut = { PORT_OUT_FORMATION_SCALE };

    override TStringArray GetVariablesOut()
    {
        return s_aVarsOut;
    }

    static override bool VisibleInPalette()
    {
        return true;
    }

    protected static override string GetOnHoverDescription()
    {
        return "HAM_AI : echelle de formation en deplacement, selon le style de combat du groupe.";
    }
}
