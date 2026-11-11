modded class SCR_InterfaceSettingsSubMenu : SCR_SettingsSubMenuBase
{
    protected const ResourceName HAM_COMPASS_SCALE_ROW_LAYOUT = "{DD7DB7E21A78E742}UI/layouts/Menus/HAM_MilsimCompassScaleRow.layout";
    protected const ResourceName HAM_COMPASS_POSITION_ROW_LAYOUT = "{56899A23286C7215}UI/layouts/Menus/HAM_MilsimCompassPositionRow.layout";
    protected const string HAM_SETTINGS_ENTRIES_WIDGET_NAME = "SettingsEntries";
    protected const string HAM_COMPASS_SCALE_SETTING_NAME = "m_fCompassScale";
    protected const ResourceName HAM_COMPASS_COLOR_ROW_LAYOUT = "{AB351EE160D890D5}UI/layouts/Menus/HAM_MilsimCompassColorRow.layout";
    protected const string HAM_COMPASS_POSITION_SETTING_NAME = "m_iCompassPosition";
    protected const string HAM_COMPASS_COLOR_SETTING_NAME = "m_iCompassColor";

    protected BaseContainer m_HAM_MilsimUserSettings;
    protected SCR_SliderComponent m_HAM_CompassScaleSlider;
    protected SCR_SpinBoxComponent m_HAM_CompassPositionSpinBox;
    protected SCR_SpinBoxComponent m_HAM_CompassColorSpinBox;

    override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
    {
        super.OnTabCreate(menuRoot, buttonsLayout, index);

        m_HAM_MilsimUserSettings = GetGame().GetGameUserSettings().GetModule("HAM_MilsimUserSettings");
        if (!m_HAM_MilsimUserSettings)
            return;

        if (!m_wRoot)
            return;

        Widget settingsEntries = m_wRoot.FindAnyWidget(HAM_SETTINGS_ENTRIES_WIDGET_NAME);
        if (!settingsEntries)
            return;

        HAM_CreatePositionRow(settingsEntries);
        HAM_CreateScaleRow(settingsEntries);
        HAM_CreateColorRow(settingsEntries);
    }

    protected void HAM_CreateColorRow(notnull Widget settingsEntries)
    {
        Widget row = GetGame().GetWorkspace().CreateWidgets(HAM_COMPASS_COLOR_ROW_LAYOUT, settingsEntries);
        if (!row)
            return;

        m_HAM_CompassColorSpinBox = SCR_SpinBoxComponent.Cast(row.FindHandler(SCR_SpinBoxComponent));
        if (!m_HAM_CompassColorSpinBox)
            return;

        int savedColor;
        m_HAM_MilsimUserSettings.Get(HAM_COMPASS_COLOR_SETTING_NAME, savedColor);

        m_HAM_CompassColorSpinBox.SetCurrentItem(savedColor);
        m_HAM_CompassColorSpinBox.m_OnChanged.Insert(HAM_OnCompassColorChanged);
    }

    protected void HAM_OnCompassColorChanged(SCR_SpinBoxComponent spinBox, int currentItem)
    {
        if (!m_HAM_MilsimUserSettings)
            return;

        m_HAM_MilsimUserSettings.Set(HAM_COMPASS_COLOR_SETTING_NAME, currentItem);
        GetGame().UserSettingsChanged();
    }

    protected void HAM_CreatePositionRow(notnull Widget settingsEntries)
    {
        Widget row = GetGame().GetWorkspace().CreateWidgets(HAM_COMPASS_POSITION_ROW_LAYOUT, settingsEntries);
        if (!row)
            return;

        m_HAM_CompassPositionSpinBox = SCR_SpinBoxComponent.Cast(row.FindHandler(SCR_SpinBoxComponent));
        if (!m_HAM_CompassPositionSpinBox)
            return;

        int savedPosition;
        m_HAM_MilsimUserSettings.Get(HAM_COMPASS_POSITION_SETTING_NAME, savedPosition);

        m_HAM_CompassPositionSpinBox.SetCurrentItem(savedPosition);
        m_HAM_CompassPositionSpinBox.m_OnChanged.Insert(HAM_OnCompassPositionChanged);
    }

    protected void HAM_CreateScaleRow(notnull Widget settingsEntries)
    {
        Widget row = GetGame().GetWorkspace().CreateWidgets(HAM_COMPASS_SCALE_ROW_LAYOUT, settingsEntries);
        if (!row)
            return;

        m_HAM_CompassScaleSlider = SCR_SliderComponent.Cast(row.FindHandler(SCR_SliderComponent));
        if (!m_HAM_CompassScaleSlider)
            return;

        float savedScale;
        m_HAM_MilsimUserSettings.Get(HAM_COMPASS_SCALE_SETTING_NAME, savedScale);

        m_HAM_CompassScaleSlider.SetValue(savedScale);
        m_HAM_CompassScaleSlider.m_OnChanged.Insert(HAM_OnCompassScaleChanged);
    }

    protected void HAM_OnCompassPositionChanged(SCR_SpinBoxComponent spinBox, int currentItem)
    {
        if (!m_HAM_MilsimUserSettings)
            return;

        m_HAM_MilsimUserSettings.Set(HAM_COMPASS_POSITION_SETTING_NAME, currentItem);
        GetGame().UserSettingsChanged();
    }

    protected void HAM_OnCompassScaleChanged(SCR_SliderComponent slider, float value)
    {
        if (!m_HAM_MilsimUserSettings)
            return;

        m_HAM_MilsimUserSettings.Set(HAM_COMPASS_SCALE_SETTING_NAME, value);
        GetGame().UserSettingsChanged();
    }
}
