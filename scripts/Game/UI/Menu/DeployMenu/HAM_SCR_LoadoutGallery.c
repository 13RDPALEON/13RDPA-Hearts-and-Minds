//------------------------------------------------------------------------------------------------
//! Galerie de tenues du menu de déploiement avec beaucoup de rôles :
//! - 4 icônes par page (place pour les flèches gauche/droite)
//! - molette réactivée pour changer de page (la vanilla la coupe)
//! - les flèches avancent d'une page, pas d'un rôle
modded class SCR_LoadoutGallery
{
	protected static const int HAM_LOADOUT_ICONS_PER_PAGE = 4;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_iCountShownItems = HAM_LOADOUT_ICONS_PER_PAGE;
		m_bCycleMode = false;
		EnablePagingInputListeners(true);
	}

	//------------------------------------------------------------------------------------------------
	override void ShowPagingButtons(bool animate = true)
	{
		bool show = m_aWidgets.Count() > m_iCountShownItems;

		if (m_PagingLeft)
			m_PagingLeft.SetVisible(show, false);

		if (m_PagingRight)
			m_PagingRight.SetVisible(show, false);

		if (!show || !m_PagingLeft || !m_PagingRight)
			return;

		int pageCount = m_aWidgets.Count();
		int currentPage = m_iSelectedItem;
		if (m_bMovePerPage && m_iCountShownItems > 0)
		{
			pageCount = Math.Ceil(pageCount / (m_iCountShownItems * 1.0));
			currentPage = Math.Floor(m_iSelectedItem / (m_iCountShownItems * 1.0));
		}

		m_PagingLeft.SetEnabled(currentPage > 0);
		m_PagingRight.SetEnabled(currentPage < pageCount - 1);
	}

	//------------------------------------------------------------------------------------------------
	override void OnNavigationLeft()
	{
		if (!m_wRoot || !m_wRoot.IsEnabledInHierarchy() || !m_wRoot.IsVisibleInHierarchy())
			return;

		int selectedItem = m_iSelectedItem - m_iCountShownItems;
		if (selectedItem < 0)
			selectedItem = 0;

		SetFocusedItem(selectedItem);
	}

	//------------------------------------------------------------------------------------------------
	override void OnNavigationRight()
	{
		if (!m_wRoot || !m_wRoot.IsEnabledInHierarchy() || !m_wRoot.IsVisibleInHierarchy())
			return;

		int selectedItem = m_iSelectedItem + m_iCountShownItems;
		int last = m_aWidgets.Count() - 1;
		if (selectedItem > last)
			selectedItem = last;

		SetFocusedItem(selectedItem);
	}
}

//------------------------------------------------------------------------------------------------
//! Le OnClick vanilla renvoie false : le clic traverse la flèche et sélectionne la tenue dessous.
modded class SCR_PagingButtonComponent
{
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);

		if (button != 0)
			return false;

		return true;
	}
}
