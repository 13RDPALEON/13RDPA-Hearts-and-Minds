// Append only our entry to each runtime Controls config, including other mods' entries.
class HAM_HoldBreathKeybinds
{
 static void Ensure(SCR_KeyBindingMenuConfig config)
 {
  if (!config || !config.m_KeyBindingCategories) return;
  SCR_KeyBindingCategory ownCategory;
  foreach (SCR_KeyBindingCategory category : config.m_KeyBindingCategories)
  {
   if (!category) continue;
   if (category.m_sName == "HAM_HoldBreath") ownCategory = category;
   if (!category.m_KeyBindingEntries) continue;
   foreach (SCR_KeyBindingEntry existing : category.m_KeyBindingEntries)
   {
    if (existing && existing.m_sActionName == "HAM_HoldBreath")
    {
     Configure(existing);
     return;
    }
   }
  }
  if (!ownCategory)
  {
   ownCategory = new SCR_KeyBindingCategory();
   ownCategory.m_sName = "HAM_HoldBreath";
   ownCategory.m_sDisplayName = "#HAM_HoldBreath_Action";
   config.m_KeyBindingCategories.Insert(ownCategory);
  }
  if (!ownCategory.m_KeyBindingEntries) ownCategory.m_KeyBindingEntries = {};
  SCR_KeyBindingEntry entry = new SCR_KeyBindingEntry();
  Configure(entry);
  ownCategory.m_KeyBindingEntries.Insert(entry);
 }
 protected static void Configure(SCR_KeyBindingEntry entry)
 {
  entry.m_sActionName = "HAM_HoldBreath";
  entry.m_sDisplayName = "#HAM_HoldBreath_Action";
  entry.m_sPreset = "hold";
  entry.m_ePrefixType = SCR_EActionPrefixType.CONTINUOUS;
  entry.m_aPlatforms = {};
 }
}
modded class SCR_KeybindSetting
{
 override protected void InsertCategoriesToComboBox()
 {
  HAM_HoldBreathKeybinds.Ensure(m_KeybindConfig);
  super.InsertCategoriesToComboBox();
 }
}
modded class SCR_SettingsManagerKeybindModule
{
 void SCR_SettingsManagerKeybindModule()
 {
  HAM_HoldBreathKeybinds.Ensure(m_KeybindConfig);
 }
}
