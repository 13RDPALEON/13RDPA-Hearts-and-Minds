class HAM_DroneSupport
{
    protected static bool s_bChecked;
    protected static bool s_bAvailable;

    static bool IsAvailable()
    {
        if (!s_bChecked)
        {
            s_bChecked = true;
            typename droneType = "HAM_ReconDroneComponent".ToType();
            if (droneType)
                s_bAvailable = true;
        }

        return s_bAvailable;
    }

    static void ReportContact(SCR_AIGroup reporter, IEntity enemy, vector pos, Faction enemyFaction)
    {
        if (!reporter || !enemy)
            return;

        SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
            reporter.FindComponent(SCR_AIGroupUtilityComponent));

        if (utility)
            utility.HAM_RelayExternalContact(enemy, pos, enemyFaction);
    }
}
