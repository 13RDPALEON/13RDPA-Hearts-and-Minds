[BaseContainerProps()]
class HAM_AIGroupFormationSetting : SCR_AIGroupFormationSettingBase
{
    protected SCR_EAIGroupFormation m_eFormation;

    static HAM_AIGroupFormationSetting Create(SCR_EAISettingOrigin origin, SCR_EAIGroupFormation formation)
    {
        HAM_AIGroupFormationSetting s = new HAM_AIGroupFormationSetting();
        s.Init(origin);
        s.m_eFormation = formation;
        return s;
    }

    override SCR_EAIGroupFormation GetFormation(int moveHandlerId, SCR_EAIGroupFormation desiredFormation)
    {
        return m_eFormation;
    }

    override string GetDebugText()
    {
        return "Formation : " + typename.EnumToString(SCR_EAIGroupFormation, m_eFormation);
    }
}
