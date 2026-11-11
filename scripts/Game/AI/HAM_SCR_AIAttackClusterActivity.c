modded class SCR_AIAttackClusterActivity
{
    override void OnActionSelected()
    {
        super.OnActionSelected();
        HAM_AssignRoles();
    }

    override void OnActionDeselected()
    {
        super.OnActionDeselected();
        HAM_ReleaseRoles();
    }

    override void OnActionCompleted()
    {
        super.OnActionCompleted();
        HAM_ReleaseRoles();
    }

    override void OnActionFailed()
    {
        super.OnActionFailed();
        HAM_ReleaseRoles();
    }

    override void OnActionRemoved()
    {
        HAM_ReleaseRoles();
        super.OnActionRemoved();
    }

    void ~SCR_AIAttackClusterActivity()
    {
        HAM_ReleaseRoles();
    }

    protected void HAM_AssignRoles()
    {
        HAM_ApplyRole(m_aFireteamsAttack, HAM_EAICombatRole.ASSAULT);
        HAM_ApplyRole(m_aFireteamsCover, HAM_EAICombatRole.COVER);
    }

    protected void HAM_ReleaseRoles()
    {
        HAM_ApplyRole(m_aFireteamsAttack, HAM_EAICombatRole.NONE);
        HAM_ApplyRole(m_aFireteamsCover, HAM_EAICombatRole.NONE);
    }

    protected void HAM_ApplyRole(array<SCR_AIGroupFireteamLock> locks, HAM_EAICombatRole role)
    {
        if (!locks)
            return;

        array<AIAgent> members = {};

        foreach (SCR_AIGroupFireteamLock lock : locks)
        {
            if (!lock)
                continue;

            SCR_AIGroupFireteam fireteam = lock.GetFireteam();
            if (!fireteam)
                continue;

            fireteam.GetMembers(members);

            foreach (AIAgent agent : members)
            {
                if (!agent)
                    continue;

                IEntity controlled = agent.GetControlledEntity();
                if (!controlled)
                    continue;

                SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(
                    controlled.FindComponent(SCR_AICombatComponent));

                if (!combat)
                    continue;

                if (role == HAM_EAICombatRole.NONE)
                    combat.HAM_ClearCombatRole(this);
                else
                    combat.HAM_SetCombatRole(role, this);
            }
        }
    }
}
