using System.Text.Json;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private readonly Dictionary<string, TextBox> _keyBoxes = new();

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;
        _controllerPanel = new Panel { Dock = DockStyle.Fill, BackColor = Panel, AutoScroll = true, Padding = new Padding(12) };
        var title = MakeLbl("SET CONTROLLER  ·  keyboard → DualShock (before Build)", TextPri, true);
        title.Location = new Point(8, 8); title.AutoSize = true;
        var hint = MakeLbl("Edit keys, then Build. This tab is removed after Build.", TextSec, false);
        hint.Location = new Point(8, 32); hint.AutoSize = true; hint.MaximumSize = new Size(720, 0);
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        _settings.KeyMap = km;
        (string prop, string label, string val)[] rows = {
            ("Cross", "Cross", km.Cross), ("Circle", "Circle", km.Circle),
            ("Square", "Square", km.Square), ("Triangle", "Triangle", km.Triangle),
            ("DpadUp", "D-Pad Up", km.DpadUp), ("DpadDown", "D-Pad Down", km.DpadDown),
            ("DpadLeft", "D-Pad Left", km.DpadLeft), ("DpadRight", "D-Pad Right", km.DpadRight),
            ("LeftStickUp", "L-Stick Up", km.LeftStickUp), ("LeftStickDown", "L-Stick Down", km.LeftStickDown),
            ("LeftStickLeft", "L-Stick Left", km.LeftStickLeft), ("LeftStickRight", "L-Stick Right", km.LeftStickRight),
            ("L1", "L1", km.L1), ("R1", "R1", km.R1), ("L2", "L2", km.L2), ("R2", "R2", km.R2),
            ("L3", "L3", km.L3), ("R3", "R3", km.R3), ("Start", "Start", km.Start), ("Select", "Select", km.Select),
        };
        int y = 60, col = 0;
        foreach (var (prop, label, val) in rows) {
            int x = 8 + col * 360;
            var lb = MakeLbl(label, TextSec, false); lb.Location = new Point(x, y); lb.AutoSize = true;
            var tb = new TextBox { Width = 140, Height = 24, Location = new Point(x + 140, y - 2),
                BackColor = Panel2, ForeColor = TextPri, BorderStyle = BorderStyle.FixedSingle, Text = val ?? "" };
            string p = prop;
            tb.Leave += (_, _) => { ApplyKeyBox(p, tb.Text); SaveSettings(); };
            _controllerPanel.Controls.Add(lb); _controllerPanel.Controls.Add(tb); _keyBoxes[prop] = tb;
            col++; if (col >= 2) { col = 0; y += 32; }
        }
        y += 48;
        var btnReset = new Ps3Button { Text = "Reset defaults", Width = 140, Height = 32, Location = new Point(8, y) };
        StyleBtn(btnReset, "Reset defaults", Panel2);
        btnReset.Click += (_, _) => {
            _settings.KeyMap = KeyMap.Defaults();
            foreach (var kv in _keyBoxes) {
                var pi = typeof(KeyMap).GetProperty(kv.Key);
                if (pi != null) kv.Value.Text = pi.GetValue(_settings.KeyMap)?.ToString() ?? "";
            }
            SaveSettings(); Append("Controller map reset to defaults.");
        };
        var btnSave = new Ps3Button { Text = "Save map", Width = 120, Height = 32, Location = new Point(160, y) };
        StyleBtn(btnSave, "Save map", Accent);
        btnSave.Click += (_, _) => {
            foreach (var kv in _keyBoxes) ApplyKeyBox(kv.Key, kv.Value.Text);
            SaveSettings(); WritePadMapForRuntime(); Append("Controller map saved.");
        };
        _controllerPanel.Controls.Add(title); _controllerPanel.Controls.Add(hint);
        _controllerPanel.Controls.Add(btnReset); _controllerPanel.Controls.Add(btnSave);
        _controllerTab = new TabPage("Controller") { BackColor = Panel, Padding = new Padding(4) };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    void ApplyKeyBox(string prop, string text) {
        if (_settings.KeyMap == null) _settings.KeyMap = KeyMap.Defaults();
        var pi = typeof(KeyMap).GetProperty(prop);
        if (pi != null && pi.CanWrite) pi.SetValue(_settings.KeyMap, (text ?? "").Trim());
    }

    void HideControllerAfterBuild() {
        if (_controllerHiddenAfterBuild) return;
        _controllerHiddenAfterBuild = true;
        try {
            if (_controllerTab != null && _tabs.TabPages.Contains(_controllerTab)) {
                if (_tabs.SelectedTab == _controllerTab) _tabs.SelectedIndex = 0;
                _tabs.TabPages.Remove(_controllerTab);
            }
            Append("Controller tab closed — mapping locked for this build.");
        } catch { }
    }

    void WritePadMapForRuntime() {
        try {
            if (_settings.KeyMap == null) _settings.KeyMap = KeyMap.Defaults();
            string path = Path.Combine(Settings.DataDir, "pad_map.json");
            File.WriteAllText(path, JsonSerializer.Serialize(_settings.KeyMap, new JsonSerializerOptions { WriteIndented = true }));
            if (!string.IsNullOrEmpty(_projectDir)) {
                string outDir = Path.Combine(_projectDir, "output");
                try { Directory.CreateDirectory(outDir); } catch { }
                File.WriteAllText(Path.Combine(outDir, "pad_map.json"), File.ReadAllText(path));
            }
        } catch { }
    }
}
