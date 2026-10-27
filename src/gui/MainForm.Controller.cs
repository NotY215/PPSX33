using System.Text.Json;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private ControllerControl? _controllerView;

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(4, 8, 16),
            Padding = new Padding(0)
        };
        _controllerView = new ControllerControl(_settings.KeyMap ?? KeyMap.Defaults())
        {
            Dock = DockStyle.Fill
        };
        _controllerView.MapChanged += (_, _) =>
        {
            _settings.KeyMap = _controllerView.CurrentMap;
            _controllerMapped = false;
            SaveSettings();
        };
        _controllerView.SaveRequested += (_, _) =>
        {
            _settings.KeyMap = _controllerView.CurrentMap;
            SaveSettings();
            WritePadMapForRuntime();
            _controllerMapped = true;
            Append("Controller mapping saved from native WinForms controller.");
            UpdateButtons();
        };
        _controllerPanel.Controls.Add(_controllerView);

        _controllerTab = new TabPage("Controller")
        {
            BackColor = Color.FromArgb(4, 8, 16),
            Padding = new Padding(0)
        };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    void WritePadMapForRuntime()
    {
        try
        {
            if (string.IsNullOrEmpty(_projectDir))
            {
                Append("Key map saved to settings; load a project to also write output/pad_map.json.");
                return;
            }
            string outDir = Path.Combine(_projectDir, "output");
            Directory.CreateDirectory(outDir);
            var km = _settings.KeyMap ?? KeyMap.Defaults();
            string path = Path.Combine(outDir, "pad_map.json");
            File.WriteAllText(path, JsonSerializer.Serialize(km, new JsonSerializerOptions { WriteIndented = true }));
            Append("Wrote pad_map.json → " + path);
        }
        catch (Exception ex) { Append("pad_map write failed: " + ex.Message); }
    }

    void HideControllerAfterBuild()
    {
        if (_controllerHiddenAfterBuild) return;
        _controllerHiddenAfterBuild = true;
        _controllerMapped = true;
        try
        {
            if (_controllerTab != null && _tabs.TabPages.Contains(_controllerTab))
            {
                if (_tabs.SelectedTab == _controllerTab) _tabs.SelectedIndex = 0;
                _tabs.TabPages.Remove(_controllerTab);
            }
            Append("Controller tab closed — mapping locked for this build.");
        }
        catch { }
    }
}
