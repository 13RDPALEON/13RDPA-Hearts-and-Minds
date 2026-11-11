//------------------------------------------------------------------------------------------------
// Zones H&M sur la carte du Conflit : même icône que les bases (symbole, nom, indicatif) et,
// au survol, le même encadré, avec les ressources H&M à la place des fournitures.
//------------------------------------------------------------------------------------------------

class HAM_MapZoneUI : SCR_CampaignMapUIElement
{
	protected static const ResourceName SUPPLY_ICON = "{CA8B528037B6831A}UI/Textures/InventoryIcons/Supplies_icon_UI.edds";
	protected static const int SIZE_HQ = 88;
	protected static const int SIZE_MAJOR = 60;
	protected static const int SIZE_BASE = 46;
	protected static const int SIZE_SMALL = 38;

	protected vector m_vPos;
	protected bool m_bSmall;
	protected bool m_bHighlighted;
	protected string m_sFactionKey;

	protected SizeLayoutWidget m_wImageOverlay;
	protected Widget m_wBaseIcon;
	protected Widget m_wServices;
	protected Widget m_wInfoText;
	protected TextWidget m_wBaseName;
	protected TextWidget m_wCallsignName;
	protected TextWidget m_wBaseNameDialog;
	protected TextWidget m_wCallsignNameDialog;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wImageOverlay = SizeLayoutWidget.Cast(w.FindAnyWidget("SizeBaseIcon"));
		m_wBaseIcon = w.FindAnyWidget("SideSymbol");
		m_wServices = w.FindAnyWidget("ServicesFrame");
		m_wInfoText = w.FindAnyWidget("Info");
		m_wBaseName = TextWidget.Cast(w.FindAnyWidget("Name"));
		m_wCallsignName = TextWidget.Cast(w.FindAnyWidget("Callsign"));
		m_wBaseNameDialog = TextWidget.Cast(w.FindAnyWidget("m_w_NameDialog"));
		m_wCallsignNameDialog = TextWidget.Cast(w.FindAnyWidget("Callsign-Dialog"));

		// Pas de services, de radio, de ralliement ni de ravitaillement Conflit sur une zone H&M.
		HAM_Hide(w, "ServicesSizeOverlay");
		HAM_Hide(w, "AntenaOff");
		HAM_Hide(w, "RallyPointIconTop");
		HAM_Hide(w, "RallyPointIconBottom");
		HAM_Hide(w, "RallyPointIconBalancer");
		HAM_Hide(w, "RallyPointText");
		HAM_Hide(w, "NextShipmentWrapper");
		HAM_Hide(w, "CaptureWarning");
		HAM_Hide(w, "LocalTask");
		HAM_Hide(w, "OpenButton");

		if (m_wServices)
			m_wServices.SetOpacity(0);

		if (m_wInfoText)
			m_wInfoText.SetVisible(false);

		SCR_MapEntity.GetOnLayerChanged().Insert(HAM_OnLayerChanged);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		SCR_MapEntity.GetOnLayerChanged().Remove(HAM_OnLayerChanged);
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_Hide(Widget root, string name)
	{
		Widget w = root.FindAnyWidget(name);
		if (w)
			w.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_Init(int index)
	{
		HAM_EMarkerKind kind = HAM_ClientMarkers.Kind(index);
		bool held = HAM_ClientMarkers.IsHeld(index);
		m_vPos = HAM_ClientMarkers.Pos(index);

		if (held)
			m_sFactionKey = HAM_Units.PLAYER_FACTION;
		else
			m_sFactionKey = HAM_Units.ENEMY_FACTION;

		int size = SIZE_BASE;
		EMilitarySymbolIcon icons = 0;
		if (kind == HAM_EMarkerKind.HQ || kind == HAM_EMarkerKind.ENEMY_HQ)
		{
			size = SIZE_HQ;
		}
		else if (kind == HAM_EMarkerKind.SLAVE)
		{
			size = SIZE_SMALL;
			m_bSmall = true;
			int slaveType = HAM_ClientMarkers.MasterType(index);
			if (slaveType == HAM_EMasterType.SUPPLY_VANILLA || slaveType == HAM_EMasterType.SUPPLY_PERSONAL)
				icons = EMilitarySymbolIcon.SUPPLY;
		}
		else if (kind == HAM_EMarkerKind.PATROL || kind == HAM_EMarkerKind.CQB || kind == HAM_EMarkerKind.IED || kind == HAM_EMarkerKind.MINES || kind == HAM_EMarkerKind.CIVILIANS)
		{
			size = SIZE_SMALL;
			m_bSmall = true;
		}
		else if (kind == HAM_EMarkerKind.DEPOSIT || kind == HAM_EMarkerKind.SUPPLY)
		{
			icons = EMilitarySymbolIcon.SUPPLY;
			m_bSmall = true;
			if (kind == HAM_EMarkerKind.SUPPLY)
				m_sFactionKey = HAM_Units.PLAYER_FACTION;
		}
		else
		{
			int type = HAM_ClientMarkers.MasterType(index);
			if (type == HAM_EMasterType.CITY || type == HAM_EMasterType.BASE)
				size = SIZE_MAJOR;
			else if (type == HAM_EMasterType.RADIO)
				icons = EMilitarySymbolIcon.SIGNAL;
			else if (type == HAM_EMasterType.FUEL)
				icons = EMilitarySymbolIcon.MAINTENANCE;
			else if (type == HAM_EMasterType.CAMP)
				icons = EMilitarySymbolIcon.INFANTRY;
			else if (type == HAM_EMasterType.HIDEOUT)
				icons = EMilitarySymbolIcon.RECON;
		}

		if (held)
			m_eIconType = SCR_EIconType.BASE;
		else
			m_eIconType = SCR_EIconType.ENEMY_BASE;

		HAM_ApplySymbol(icons, size);

		m_sName = HAM_ClientMarkers.Label(index);
		string subtitle = HAM_ClientMarkers.Subtitle(index);
		HAM_SetText(m_wBaseName, m_sName);
		HAM_SetText(m_wBaseNameDialog, m_sName);
		// Sous le nom, l'indicatif comme une base du Conflit ; au survol, l'indicatif puis le type de zone.
		string callsign = HAM_Callsigns.Of(m_sName);
		if (callsign.IsEmpty())
		{
			HAM_SetText(m_wCallsignName, subtitle);
			HAM_SetText(m_wCallsignNameDialog, subtitle);
		}
		else
		{
			HAM_SetText(m_wCallsignName, callsign);
			HAM_SetText(m_wCallsignNameDialog, WidgetManager.Translate(callsign) + "  ·  " + subtitle);
		}

		int stockType = HAM_ClientMarkers.MasterType(index);
		if (stockType == HAM_EMasterType.SUPPLY_VANILLA || stockType == HAM_EMasterType.SUPPLY_PERSONAL)
		{
			string stockLine = HAM_ClientMarkers.StockLine(index);
			HAM_SetText(m_wCallsignName, stockLine);
			HAM_SetText(m_wCallsignNameDialog, stockLine);
		}

		HAM_FillInfo(index);

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity)
			HAM_OnLayerChanged(mapEntity.GetLayerIndex());
	}

	//------------------------------------------------------------------------------------------------
	//! Symbole militaire, couleur de faction et contours de survol, comme une base du Conflit.
	protected void HAM_ApplySymbol(EMilitarySymbolIcon icons, int size)
	{
		EMilitarySymbolIdentity identity = EMilitarySymbolIdentity.UNKNOWN;
		string selection = "Unknown_Select";
		string highlight = "Unknown_Installation_Focus_Land";
		Color nameColor = Color.FromRGBA(249, 210, 103, 255);

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (campaign && m_sFactionKey == campaign.GetFactionKeyByEnum(SCR_ECampaignFaction.OPFOR))
		{
			identity = EMilitarySymbolIdentity.OPFOR;
			selection = "Hostile_Select";
			highlight = "Hostile_Installation_Focus_Land";
			nameColor = Color.FromRGBA(238, 49, 47, 255);
		}
		else if (campaign && m_sFactionKey == campaign.GetFactionKeyByEnum(SCR_ECampaignFaction.BLUFOR))
		{
			identity = EMilitarySymbolIdentity.BLUFOR;
			selection = "Friend_Select";
			highlight = "Friend_Installation_Focus_Land";
			nameColor = Color.FromRGBA(31, 195, 243, 255);
		}

		if (m_wBaseIcon)
		{
			Color color = GetColorForFaction(m_sFactionKey);
			if (color)
			{
				m_wBaseIcon.SetColor(color);
				if (m_wGradient)
					m_wGradient.SetColor(color);
			}

			SCR_MilitarySymbolUIComponent symbolUI = SCR_MilitarySymbolUIComponent.Cast(m_wBaseIcon.FindHandler(SCR_MilitarySymbolUIComponent));
			if (symbolUI)
			{
				SCR_MilitarySymbol symbol = new SCR_MilitarySymbol();
				symbol.SetIdentity(identity);
				symbol.SetDimension(2);
				symbol.SetIcons(icons);
				symbolUI.Update(symbol);
			}
		}

		if (m_wHighlightImg)
			m_wHighlightImg.LoadImageFromSet(0, m_sImageSetARO, highlight);

		if (m_wSelectImg)
			m_wSelectImg.LoadImageFromSet(0, m_sImageSetARO, selection);

		if (m_wImageOverlay)
		{
			m_wImageOverlay.SetWidthOverride(size);
			m_wImageOverlay.SetHeightOverride(size);
		}

		if (m_wBaseNameDialog)
			m_wBaseNameDialog.SetColor(nameColor);
	}

	//------------------------------------------------------------------------------------------------
	//! Encadré du survol : ressources sur la ligne des fournitures, hommes et grâce sur la ligne suivante.
	protected void HAM_FillInfo(int index)
	{
		if (!m_wInfoText)
			return;

		RichTextWidget supplies = RichTextWidget.Cast(m_wInfoText.FindAnyWidget("Supplies"));
		RichTextWidget respawn = RichTextWidget.Cast(m_wInfoText.FindAnyWidget("Respawn"));
		RichTextWidget freq = RichTextWidget.Cast(m_wInfoText.FindAnyWidget("Freq"));
		ImageWidget suppliesImg = ImageWidget.Cast(m_wInfoText.FindAnyWidget("SuppliesIMG"));
		ImageWidget respawnImg = ImageWidget.Cast(m_wInfoText.FindAnyWidget("RespawnIMG"));
		ImageWidget freqImg = ImageWidget.Cast(m_wInfoText.FindAnyWidget("FreqIMG"));

		if (freq)
			freq.SetVisible(false);

		if (freqImg)
			freqImg.SetVisible(false);

		if (respawnImg)
			respawnImg.SetVisible(false);

		if (suppliesImg)
			suppliesImg.LoadImageTexture(0, SUPPLY_ICON);

		if (supplies)
		{
			supplies.SetText(HAM_ClientMarkers.StockLine(index));
			supplies.SetVisible(true);
		}

		if (respawn)
		{
			string menLine = HAM_ClientMarkers.MenLine(index);
			respawn.SetText(menLine);
			respawn.SetVisible(!menLine.IsEmpty());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_SetText(TextWidget widget, string text)
	{
		if (widget)
			widget.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_OnLayerChanged(int layer)
	{
		if (m_bSmall && m_wBaseName)
			m_wBaseName.SetVisible(layer <= 3);
	}

	//------------------------------------------------------------------------------------------------
	override void ShowName(bool visible)
	{
		if (m_wBaseName)
			m_wBaseName.SetVisible(visible);

		if (m_wCallsignName)
			m_wCallsignName.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	override vector GetPos()
	{
		return m_vPos;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		super.OnMouseEnter(w, x, y);
		m_bHighlighted = true;

		if (w.Type() == ButtonWidget)
			AnimExpand();

		if (m_wServices)
			m_wServices.SetVisible(true);

		if (m_wInfoText)
			m_wInfoText.SetVisible(true);

		m_wRoot.SetZOrder(1);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		super.OnMouseLeave(w, enterW, x, y);
		AnimCollapse();
		m_bHighlighted = false;

		if (m_wInfoText)
			m_wInfoText.SetVisible(false);

		m_wRoot.SetZOrder(0);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override protected void AnimExpand()
	{
		if (!m_bHighlighted)
			return;

		if (m_eIconType == SCR_EIconType.ENEMY_BASE)
			PlayHoverSound(m_sSoundEnemyBase);
		else
			PlayHoverSound(m_sSoundBase);

		if (m_wServices)
			AnimateWidget.Opacity(m_wServices, 1, ANIM_SPEED);

		if (m_wHighlightImg)
			m_wHighlightImg.SetVisible(true);

		if (m_wGradient)
			m_wGradient.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	override protected void AnimCollapse()
	{
		if (m_wHighlightImg)
			m_wHighlightImg.SetVisible(false);

		if (m_wGradient && !m_bIsSelected)
			m_wGradient.SetVisible(false);

		if (m_wServices)
			AnimateWidget.Opacity(m_wServices, 0, ANIM_SPEED);
	}
}

//------------------------------------------------------------------------------------------------
//! Texte des ressources et des hommes, posé au-dessus de l'icône de zone.
class HAM_MapZoneInfo : SCR_MapUIElement
{
	static const ResourceName LAYOUT = "{D597FD53A7FDC815}UI/layouts/HAM/HAM_ZoneInfo.layout";

	protected vector m_vPos;
	protected TextWidget m_wInfo;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wInfo = TextWidget.Cast(w.FindAnyWidget("Info"));
	}

	void HAM_Init(int index, string text)
	{
		m_vPos = HAM_ClientMarkers.Pos(index);
		if (m_wInfo)
			m_wInfo.SetText(text);

		if (m_wRoot)
		{
			m_wRoot.SetZOrder(30);
			m_wRoot.SetVisible(true);
		}
	}

	override vector GetPos()
	{
		return m_vPos;
	}
}

//------------------------------------------------------------------------------------------------
//! La carte du Conflit dessine aussi les zones H&M, avec le même conteneur que ses bases.
modded class SCR_MapCampaignUI
{
	protected static const ResourceName HAM_ZONE_LAYOUT = "{D597FD53A7FDC814}UI/layouts/HAM/HAM_ZoneElement.layout";
	protected static SCR_MapCampaignUI s_HAM_OpenMap;
	protected static float s_fHAM_StateAsk;

	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		s_HAM_OpenMap = this;
		HAM_BuildZoneIcons();
		HAM_AskFrontState(true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		if (s_HAM_OpenMap == this)
			s_HAM_OpenMap = null;

		super.OnMapClose(config);
	}

	//------------------------------------------------------------------------------------------------
	//! Demande les stocks au serveur. L'ouverture de carte force l'envoi ; le survol est espacé.
	protected void HAM_AskFrontState(bool force)
	{
		float now = HAM_Util.Now();
		if (!force && now - s_fHAM_StateAsk < 3)
			return;

		s_fHAM_StateAsk = now;
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_AskFrontState();
	}

	//------------------------------------------------------------------------------------------------
	//! Nouvelles données du serveur pendant que la carte est ouverte.
	static void HAM_RefreshOpenMap()
	{
		if (s_HAM_OpenMap)
			s_HAM_OpenMap.HAM_BuildZoneIcons();
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_BuildZoneIcons()
	{
		array<Widget> previous = {};
		foreach (Widget w, SCR_MapUIElement icon : m_mIcons)
		{
			if (HAM_MapZoneUI.Cast(icon) || HAM_MapZoneInfo.Cast(icon))
			{
				previous.Insert(w);
				continue;
			}

			SCR_CampaignMapUIBase baseIcon = SCR_CampaignMapUIBase.Cast(icon);
			if (baseIcon && baseIcon.GetBase())
				baseIcon.SetIconInfoText();
		}

		foreach (Widget old : previous)
		{
			m_mIcons.Remove(old);
			old.RemoveFromHierarchy();
		}

		if (!m_wIconsContainer || !HAM_Mission.IsActive())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int count = HAM_ClientMarkers.Count();
		for (int i = 0; i < count; i++)
		{
			if (HAM_ClientMarkers.ShownByConflict(i))
				continue;

			Widget created = workspace.CreateWidgets(HAM_ZONE_LAYOUT, m_wIconsContainer);
			if (!created)
				continue;

			HAM_MapZoneUI handler = HAM_MapZoneUI.Cast(created.FindHandler(HAM_MapZoneUI));
			if (!handler)
			{
				created.RemoveFromHierarchy();
				continue;
			}

			handler.SetParent(this);
			handler.HAM_Init(i);
			m_mIcons.Set(created, handler);
			FrameSlot.SetSizeToContent(created, true);
			FrameSlot.SetAlignment(created, 0.5, 0.5);
		}

		HAM_BuildInfoMarkers(workspace);
		UpdateIcons();
	}

	//------------------------------------------------------------------------------------------------
	//! Marqueur de texte au-dessus de l'icône, y compris quand la base Conflit dessine déjà la zone.
	protected void HAM_BuildInfoMarkers(WorkspaceWidget workspace)
	{
		if (!workspace || !m_wIconsContainer)
			return;

		int count = HAM_ClientMarkers.Count();
		for (int i = 0; i < count; i++)
		{
			string info = HAM_ClientMarkers.InfoLine(i);
			if (info.IsEmpty())
				continue;

			Widget created = workspace.CreateWidgets(HAM_MapZoneInfo.LAYOUT, m_wIconsContainer);
			if (!created)
				continue;

			HAM_MapZoneInfo handler = HAM_MapZoneInfo.Cast(created.FindHandler(HAM_MapZoneInfo));
			if (!handler)
			{
				created.RemoveFromHierarchy();
				continue;
			}

			handler.SetParent(this);
			handler.HAM_Init(i, info);
			m_mIcons.Set(created, handler);
			FrameSlot.SetSizeToContent(created, true);
			FrameSlot.SetAlignment(created, 0.5, 0.5);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected override void UpdateIconPosition(Widget widget, SCR_MapUIElement icon, float x, float y)
	{
		HAM_MapZoneInfo info = HAM_MapZoneInfo.Cast(icon);
		if (info)
		{
			FrameSlot.SetPos(widget, x, y - 108);
			return;
		}

		super.UpdateIconPosition(widget, icon, x, y);
	}
}

//------------------------------------------------------------------------------------------------
//! QG et QG désignés : l'icône reste celle de la base Conflit, l'encadré ajoute les ressources H&M.
modded class SCR_CampaignMilitaryBaseMapDescriptorComponent
{
	override void MapSetup(notnull Faction faction)
	{
		super.MapSetup(faction);
		if (m_CampaignBase && HAM_MasterBases.MapHidden(m_CampaignBase))
			HAM_MasterBases.ApplyMapVisibility(m_CampaignBase, false);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignMapUIBase
{
	override void InitBase(SCR_CampaignMilitaryBaseComponent base)
	{
		super.InitBase(base);
		if (HAM_MasterBases.MapHidden(base))
			SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! La carte réaffiche le widget à l'ouverture et au changement de zoom.
	override void SetVisible(bool visible)
	{
		if (m_Base && HAM_MasterBases.MapHidden(m_Base))
			visible = false;

		super.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	override void ShowName(bool visible)
	{
		if (m_Base && HAM_MasterBases.MapHidden(m_Base))
			visible = false;

		super.ShowName(visible);
	}

	//------------------------------------------------------------------------------------------------
	override void ChangeNameSize(bool visible)
	{
		if (m_Base && HAM_MasterBases.MapHidden(m_Base))
		{
			super.ChangeNameSize(false);
			SetVisible(false);
			return;
		}

		super.ChangeNameSize(visible);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_ApplyZoneName(string name)
	{
		if (name.IsEmpty())
			return;

		SetIconName(name);
	}

	//------------------------------------------------------------------------------------------------
	//! Base Conflit sans indicatif affiché (QG posés par H&M) : celui de la zone H&M.
	override protected void SetIconName(string name)
	{
		super.SetIconName(name);

		if (!m_wCallsignName || !HAM_Mission.IsActive() || !m_wCallsignName.GetText().IsEmpty())
			return;

		string callsign = HAM_Callsigns.Of(name);
		if (callsign.IsEmpty())
			return;

		m_wCallsignName.SetText(callsign);
		if (m_wCallsignNameDialog)
			m_wCallsignNameDialog.SetText(callsign);
	}

	//------------------------------------------------------------------------------------------------
	override void SetIconInfoText()
	{
		super.SetIconInfoText();

		if (!m_Base || !m_Base.GetOwner())
			return;

		if (HAM_ClientMarkers.Count() <= 0)
		{
			HAM_ClientMarkers.RequestIfEmpty();
			return;
		}

		Faction baseFaction = m_Base.GetFaction();
		Faction playerFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (baseFaction && playerFaction && baseFaction.GetFactionKey() != playerFaction.GetFactionKey())
			return;

		int zone = HAM_ClientMarkers.ZoneAtBase(m_Base.GetOwner().GetOrigin());
		if (zone < 0)
			zone = HAM_ClientMarkers.ZoneByLabel(m_Base.GetBaseName());

		if (zone < 0)
			return;

		if (!m_wRoot)
			return;

		Widget info = m_wRoot.FindAnyWidget("Info");
		if (!info)
			return;

		string extra = HAM_ClientMarkers.StockLine(zone);
		string menLine = HAM_ClientMarkers.MenLine(zone);

		TextWidget supplies = TextWidget.Cast(info.FindAnyWidget("Supplies"));
		if (supplies)
		{
			string supplyText = supplies.GetText();
			string shown = extra;
			if (!supplyText.IsEmpty())
				shown = "Fournitures " + supplyText;

			if (!extra.IsEmpty())
			{
				if (!shown.IsEmpty())
					shown = shown + "\n" + extra;
				else
					shown = extra;
			}

			supplies.SetText(shown);
			supplies.SetVisible(true);
		}

		TextWidget respawn = TextWidget.Cast(info.FindAnyWidget("Respawn"));
		if (!respawn)
			return;

		string current = respawn.GetText();
		if (current.Contains(":"))
			return;

		respawn.SetText(menLine);
		respawn.SetVisible(!menLine.IsEmpty());
	}
}
