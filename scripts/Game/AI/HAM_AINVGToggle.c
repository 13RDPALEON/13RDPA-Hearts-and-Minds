class HAM_AINVGToggle
{
    static const float CHECK_PERIOD_MS = 15000;

    static const bool TRACE = false;

    protected static bool s_bNight;

    static bool IsNight()
    {
        return s_bNight;
    }

    protected static void RefreshPerception(IEntity character)
    {
        if (!character)
            return;

        SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(character.FindComponent(SCR_AICombatComponent));
        if (combat)
            combat.HAM_RefreshEquipmentPerception();
    }

    protected static bool ToggleCharacter(IEntity character, bool newState, inout int carried, inout int lowered)
    {
        if (!character)
            return false;

        SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(character);
        if (!gadgetManager)
            return false;

        array<SCR_GadgetComponent> gadgets = gadgetManager.GetGadgetsByType(EGadgetType.NIGHT_VISION);
        if (!gadgets || gadgets.IsEmpty())
            return false;

        carried = carried + 1;

        bool changed = false;
        foreach (SCR_GadgetComponent gadget : gadgets)
        {
            if (!gadget)
                continue;

            if (gadget.IsToggledOn())
                lowered = lowered + 1;

            if (gadget.IsToggledOn() == newState)
                continue;

            if (newState && gadget.GetMode() != EGadgetMode.IN_SLOT)
                gadgetManager.SetGadgetMode(gadget.GetOwner(), EGadgetMode.IN_SLOT);

            gadget.ToggleActive(newState, gadget.GetUseMask());

            if (TRACE)
                PrintFormat("[HAM_AI][NVG] %1 : mode=%2 allumee=%3 demande=%4",
                    HAM_AILookRules.PrefabName(character), gadget.GetMode(), gadget.IsToggledOn(), newState);

            changed = true;
        }

        if (changed)
            RefreshPerception(character);

        return changed;
    }

    static void RefreshAllPerception()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (agent)
                RefreshPerception(agent.GetControlledEntity());
        }
    }

    static void Sweep()
    {
        ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
        if (!world)
            return;

        TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
        if (!timeManager)
            return;

        float sunrise, sunset;
        if (!timeManager.GetSunriseHour(sunrise) || !timeManager.GetSunsetHour(sunset))
            return;

        float hour = timeManager.GetTimeOfTheDay();
        bool night = hour < sunrise || hour > sunset;

        bool dayNightFlipped = night != s_bNight;
        s_bNight = night;

        bool toggleAllowed = HAM_AISettingsComponent.HAM_NVGAuto();

        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        int changed = 0;
        int carried = 0;
        int lowered = 0;

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity character = agent.GetControlledEntity();

            if (toggleAllowed && ToggleCharacter(character, night, carried, lowered))
                changed++;
            else if (dayNightFlipped)
                RefreshPerception(character);
        }

        if (TRACE && !agents.IsEmpty())
            PrintFormat("[HAM_AI][NVG] balayage : nuit=%1 agents=%2 porteurs=%3 descendues=%4 bascules=%5 (lever %6 coucher %7)",
                night, agents.Count(), carried, lowered, changed, sunrise, sunset);
    }
}
