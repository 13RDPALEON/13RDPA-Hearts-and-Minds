class HAM_MilsimCompassDisplay : SCR_InfoDisplay
{
    protected const int GRADUATION_COUNT = 13;
    protected const float DEGREES_PER_GRADUATION = 15;
    protected const float PIXELS_PER_DEGREE = 3.85;
    protected const float GRADUATION_WIDTH = 42;
    protected const float GRADUATION_HALF_WIDTH = 21;
    protected const float GRADUATION_POSITION_Y = 11;
    protected const float GRADUATION_HEIGHT = 17;
    protected const float CURSOR_POSITION_X = -14;
    protected const float CURSOR_WIDTH = 28;
    protected const float CURSOR_POSITION_Y = 0;
    protected const float CURSOR_HEIGHT = 13;
    protected const float BEARING_POSITION_X = -35;
    protected const float BEARING_WIDTH = 70;
    protected const float BEARING_POSITION_Y = 25;
    protected const float BEARING_HEIGHT = 14;
    protected const float FRAME_WIDTH = 560;
    protected const float FRAME_HEIGHT = 39;
    protected const float FRAME_TOP_MARGIN = 16;
    protected const float FRAME_BOTTOM_MARGIN = -90;
    protected const int POSITION_BOTTOM = 1;
    protected const int GRADUATION_FONT_SIZE = 13;
    protected const int CURSOR_FONT_SIZE = 11;
    protected const int BEARING_FONT_SIZE = 14;
    protected const int SHADOW_SIZE = 1;
    protected const int SHADOW_DARK = 0xFF000000;

    protected const int COLOR_INDEX_BLACK = 1;
    protected const string TOGGLE_ACTION_NAME = "HAM_MilsimToggleCompass";

    protected ref array<TextWidget> m_aGraduationWidgets;
    protected ref array<string> m_aGraduationLabels;
    protected Widget m_wClipWidget;
    protected TextWidget m_wCursorWidget;
    protected TextWidget m_wBearingWidget;
    protected string m_sBearingLabel;
    protected bool m_bHiddenByPlayer;
    protected bool m_bActionListenerAdded;
    protected int m_iAppliedColorIndex;
    protected float m_fAppliedScale;
    protected float m_fPlayerScale;
    protected int m_iAppliedPosition;
    protected int m_iPlayerPosition;
    protected int m_iPlayerColor;

    override protected event void OnStartDraw(IEntity owner)
    {
        super.OnStartDraw(owner);

        if (!m_wRoot)
            return;

        m_aGraduationWidgets = new array<TextWidget>();
        for (int i = 0; i < GRADUATION_COUNT; i++)
        {
            TextWidget graduationWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("HAM_CompassGrad" + i.ToString()));
            if (graduationWidget)
                m_aGraduationWidgets.Insert(graduationWidget);
        }

        m_wClipWidget = m_wRoot.FindAnyWidget("HAM_CompassClip");
        m_wCursorWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("HAM_CompassCursor"));
        m_wBearingWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("HAM_CompassBearing"));
        m_iAppliedColorIndex = -1;
        m_fAppliedScale = -1;
        m_iAppliedPosition = -1;
        m_bHiddenByPlayer = false;
        ReadPlayerSettings();
        GetGame().OnUserSettingsChangedInvoker().Insert(OnUserSettingsChanged);

        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager)
        {
            inputManager.AddActionListener(TOGGLE_ACTION_NAME, EActionTrigger.DOWN, OnToggleActionTriggered);
            m_bActionListenerAdded = true;
        }
    }

    override protected event void OnStopDraw(IEntity owner)
    {
        GetGame().OnUserSettingsChangedInvoker().Remove(OnUserSettingsChanged);

        if (m_bActionListenerAdded)
        {
            InputManager inputManager = GetGame().GetInputManager();
            if (inputManager)
                inputManager.RemoveActionListener(TOGGLE_ACTION_NAME, EActionTrigger.DOWN, OnToggleActionTriggered);

            m_bActionListenerAdded = false;
        }

        m_aGraduationWidgets = null;
        m_wClipWidget = null;
        m_wCursorWidget = null;
        m_wBearingWidget = null;

        super.OnStopDraw(owner);
    }

    protected void OnToggleActionTriggered()
    {
        m_bHiddenByPlayer = !m_bHiddenByPlayer;
    }

    override protected event void UpdateValues(IEntity owner, float timeSlice)
    {
        super.UpdateValues(owner, timeSlice);

        if (!m_wRoot || !m_aGraduationWidgets)
            return;

        bool shouldShow = HAM_MilsimSettingsComponent.HAM_ShowCompass() && !m_bHiddenByPlayer;
        m_wRoot.SetVisible(shouldShow);

        if (!shouldShow)
            return;

        ApplyColor();
        ApplyGeometry();
        UpdateGraduations();
    }

    protected void UpdateGraduations()
    {
        BaseWorld world = GetGame().GetWorld();
        if (!world)
            return;

        vector cameraTransform[4];
        world.GetCurrentCamera(cameraTransform);

        float bearing = Math3D.MatrixToAngles(cameraTransform)[0];
        if (bearing < 0)
            bearing = bearing + 360;

        if (m_wBearingWidget)
        {
            string bearingText = FormatBearing(Math.Round(bearing));
            if (bearingText != m_sBearingLabel)
            {
                m_sBearingLabel = bearingText;
                m_wBearingWidget.SetText(bearingText);
            }
        }

        int centerStep = Math.Round(bearing / DEGREES_PER_GRADUATION);
        int sideCount = GRADUATION_COUNT / 2;

        if (!m_aGraduationLabels)
            m_aGraduationLabels = {};

        for (int i = 0; i < m_aGraduationWidgets.Count(); i++)
        {
            float graduationAngle = (centerStep + i - sideCount) * DEGREES_PER_GRADUATION;
            float offsetFromCenter = graduationAngle - bearing;

            TextWidget graduationWidget = m_aGraduationWidgets[i];
            string label = GraduationLabel(graduationAngle);
            if (i >= m_aGraduationLabels.Count())
                m_aGraduationLabels.Insert(string.Empty);

            if (m_aGraduationLabels[i] != label)
            {
                m_aGraduationLabels[i] = label;
                graduationWidget.SetText(label);
            }

            FrameSlot.SetPosX(graduationWidget, (offsetFromCenter * PIXELS_PER_DEGREE - GRADUATION_HALF_WIDTH) * m_fAppliedScale);
        }
    }

    protected string GraduationLabel(float angle)
    {
        int normalizedAngle = NormalizeAngle(Math.Round(angle));

        if (normalizedAngle == 0)
            return "N";
        if (normalizedAngle == 45)
            return "NE";
        if (normalizedAngle == 90)
            return "E";
        if (normalizedAngle == 135)
            return "SE";
        if (normalizedAngle == 180)
            return "S";
        if (normalizedAngle == 225)
            return "SO";
        if (normalizedAngle == 270)
            return "O";
        if (normalizedAngle == 315)
            return "NO";

        return FormatBearing(normalizedAngle);
    }

    protected int NormalizeAngle(int angle)
    {
        int normalizedAngle = angle;
        while (normalizedAngle < 0)
        {
            normalizedAngle = normalizedAngle + 360;
        }
        while (normalizedAngle >= 360)
        {
            normalizedAngle = normalizedAngle - 360;
        }
        return normalizedAngle;
    }

    protected string FormatBearing(int bearing)
    {
        int normalizedBearing = NormalizeAngle(bearing);

        if (normalizedBearing < 10)
            return "00" + normalizedBearing.ToString();
        if (normalizedBearing < 100)
            return "0" + normalizedBearing.ToString();

        return normalizedBearing.ToString();
    }

    protected void ReadPlayerSettings()
    {
        m_fPlayerScale = 100;
        m_iPlayerPosition = 0;
        m_iPlayerColor = 0;

        BaseContainer settingsContainer = GetGame().GetGameUserSettings().GetModule("HAM_MilsimUserSettings");
        if (!settingsContainer)
            return;

        HAM_MilsimUserSettings userSettings = new HAM_MilsimUserSettings();
        BaseContainerTools.WriteToInstance(userSettings, settingsContainer);

        if (userSettings.m_fCompassScale >= 1)
            m_fPlayerScale = userSettings.m_fCompassScale;

        m_iPlayerPosition = userSettings.m_iCompassPosition;
        m_iPlayerColor = userSettings.m_iCompassColor;
    }

    protected void OnUserSettingsChanged()
    {
        ReadPlayerSettings();
        m_fAppliedScale = -1;
        m_iAppliedColorIndex = -1;
    }

    protected void ApplyGeometry()
    {
        float scale = m_fPlayerScale / 100.0;
        if (scale < 0.1)
            scale = 0.1;

        if (Math.AbsFloat(scale - m_fAppliedScale) < 0.001 && m_iPlayerPosition == m_iAppliedPosition)
            return;

        m_fAppliedScale = scale;
        m_iAppliedPosition = m_iPlayerPosition;

        if (m_wClipWidget)
        {
            float anchorY = 0;
            float positionY = FRAME_TOP_MARGIN;

            if (m_iPlayerPosition == POSITION_BOTTOM)
            {
                anchorY = 1;
                positionY = FRAME_BOTTOM_MARGIN;
            }

            float scaledWidth = FRAME_WIDTH * scale;
            FrameSlot.SetAnchor(m_wClipWidget, 0.5, anchorY);
            FrameSlot.SetSize(m_wClipWidget, scaledWidth, FRAME_HEIGHT * scale);
            FrameSlot.SetPos(m_wClipWidget, -scaledWidth / 2, positionY);
        }

        foreach (TextWidget graduationWidget : m_aGraduationWidgets)
        {
            FrameSlot.SetSize(graduationWidget, GRADUATION_WIDTH * scale, GRADUATION_HEIGHT * scale);
            FrameSlot.SetPosY(graduationWidget, GRADUATION_POSITION_Y * scale);
            graduationWidget.SetExactFontSize(Math.Round(GRADUATION_FONT_SIZE * scale));
        }

        if (m_wCursorWidget)
        {
            FrameSlot.SetSize(m_wCursorWidget, CURSOR_WIDTH * scale, CURSOR_HEIGHT * scale);
            FrameSlot.SetPos(m_wCursorWidget, CURSOR_POSITION_X * scale, CURSOR_POSITION_Y * scale);
            m_wCursorWidget.SetExactFontSize(Math.Round(CURSOR_FONT_SIZE * scale));
        }

        if (m_wBearingWidget)
        {
            FrameSlot.SetSize(m_wBearingWidget, BEARING_WIDTH * scale, BEARING_HEIGHT * scale);
            FrameSlot.SetPos(m_wBearingWidget, BEARING_POSITION_X * scale, BEARING_POSITION_Y * scale);
            m_wBearingWidget.SetExactFontSize(Math.Round(BEARING_FONT_SIZE * scale));
        }
    }

    protected void ApplyColor()
    {
        int colorIndex = m_iPlayerColor;
        if (colorIndex == m_iAppliedColorIndex)
            return;

        m_iAppliedColorIndex = colorIndex;

        int textColor = ColorFromIndex(colorIndex);
        int shadowSize = SHADOW_SIZE;
        if (colorIndex == COLOR_INDEX_BLACK)
            shadowSize = 0;

        foreach (TextWidget graduationWidget : m_aGraduationWidgets)
        {
            graduationWidget.SetColorInt(textColor);
            graduationWidget.SetShadow(shadowSize, SHADOW_DARK);
        }

        if (m_wCursorWidget)
        {
            m_wCursorWidget.SetColorInt(textColor);
            m_wCursorWidget.SetShadow(shadowSize, SHADOW_DARK);
        }

        if (m_wBearingWidget)
        {
            m_wBearingWidget.SetColorInt(textColor);
            m_wBearingWidget.SetShadow(shadowSize, SHADOW_DARK);
        }
    }

    protected int ColorFromIndex(int colorIndex)
    {
        if (colorIndex == 1)
            return Color.BLACK;
        if (colorIndex == 2)
            return Color.RED;
        if (colorIndex == 3)
            return Color.GREEN;
        if (colorIndex == 4)
            return Color.BLUE;
        if (colorIndex == 5)
            return Color.YELLOW;
        if (colorIndex == 6)
            return Color.CYAN;
        if (colorIndex == 7)
            return Color.ORANGE;

        return Color.WHITE;
    }
}
