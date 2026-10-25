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

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Panel,
            AutoScroll = true,
            Padding = new Padding(16)
        };

        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            ColumnCount = 1,
            BackColor = Panel,
            Padding = new Padding(0)
        };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        var title = MakeLbl("SET CONTROLLER  ·  keyboard → DualShock (before Build)", TextPri, true);
        title.AutoSize = true;
        title.Margin = new Padding(0, 0, 0, 4);
        var hint = MakeLbl("Edit keys, then Build. This tab is removed after Build.", TextSec, false);
        hint.AutoSize = true;
        hint.Margin = new Padding(0, 0, 0, 12);

        root.Controls.Add(title);
        root.Controls.Add(hint);

        // button row
        var btnRow = new FlowLayoutPanel
        {
            AutoSize = true,
            FlowDirection = FlowDirection.LeftToRight,
            WrapContents = false,
            BackColor = Color.Transparent,
            Margin = new Padding(0, 0, 0, 16)
        };
        var btnSave = new Ps3Button { Width = 140, Height = 36 };
        StyleBtnNoDock(btnSave, "Save map", Accent);
        btnSave.Click += (_, _) =>
        {
            foreach (var kv in _keyBoxes) ApplyKeyBox(kv.Key, kv.Value.Text);
            SaveSettings();
            WritePadMapForRuntime();
            Append("Controller map saved.");
        };
        var btnReset = new Ps3Button { Width = 140, Height = 36 };
        StyleBtnNoDock(btnReset, "Reset defaults", Panel2);
        btnReset.Click += (_, _) =>
        {
            _settings.KeyMap = KeyMap.Defaults();
            foreach (var kv in _keyBoxes)
            {
                var pi = typeof(KeyMap).GetProperty(kv.Key);
                if (pi != null) kv.Value.Text = pi.GetValue(_settings.KeyMap)?.ToString() ?? "";
            }
            SaveSettings();
            Append("Controller map reset to defaults.");
        };
        btnRow.Controls.Add(btnSave);
        btnRow.Controls.Add(btnReset);
        root.Controls.Add(btnRow);

        var km = _settings.KeyMap ?? KeyMap.Defaults();
        _settings.KeyMap = km;
        (string prop, string label, string val)[] rows = {
            ("Cross", "Cross", km.Cross), ("Circle", "Circle", km.Circle),
            ("Square", "Square", km.Square), ("Triangle", "Triangle", km.Triangle),
            ("DpadUp", "D-Pad Up", km.DpadUp), ("DpadDown", "D-Pad Down", km.DpadDown),
            ("DpadLeft", "D-Pad Left", km.DpadLeft), ("DpadRight", "D-Pad Right", km.DpadRight),
            ("LeftStickUp", "L-Stick Up", km.LeftStickUp), ("LeftStickDown", "L-Stick Down", km.LeftStickDown),
            ("LeftStickLeft", "L-Stick Left", km.LeftStickLeft), ("LeftStickRight", "L-Stick Right", km.LeftStickRight),
            ("L1", "L1", km.L1), ("R1", "R1", km.R1),
            ("L2", "L2", km.L2), ("R2", "R2", km.R2),
            ("L3", "L3", km.L3), ("R3", "R3", km.R3),
            ("Start", "Start", km.Start), ("Select", "Select", km.Select),
        };

        var grid = new TableLayoutPanel
        {
            AutoSize = true,
            ColumnCount = 4,
            BackColor = Panel,
            Margin = new Padding(0)
        };
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 160));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 160));

        int r = 0;
        for (int i = 0; i < rows.Length; i += 2)
        {
            grid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34));
            AddKeyCell(grid, rows[i].prop, rows[i].label, rows[i].val, 0, r);
            if (i + 1 < rows.Length)
                AddKeyCell(grid, rows[i + 1].prop, rows[i + 1].label, rows[i + 1].val, 2, r);
            r++;
        }
        root.Controls.Add(grid);
        _controllerPanel.Controls.Add(root);

        _controllerTab = new TabPage("Controller") { BackColor = Panel, Padding = new Padding(4) };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    void AddKeyCell(TableLayoutPanel grid, string prop, string label, string val, int col, int row)
    {
        var lb = MakeLbl(label, TextSec, false);
        lb.AutoSize = true;
        lb.Anchor = AnchorStyles.Left;
        lb.Margin = new Padding(0, 6, 8, 0);
        var tb = new TextBox
        {
            Width = 150,
            Height = 26,
            BackColor = Panel2,
            ForeColor = TextPri,
            BorderStyle = BorderStyle.FixedSingle,
            Text = val ?? "",
            Margin = new Padding(0, 4, 16, 4)
        };
        string p = prop;
        tb.Leave += (_, _) => { ApplyKeyBox(p, tb.Text); SaveSettings(); };
        grid.Controls.Add(lb, col, row);
        grid.Controls.Add(tb, col + 1, row);
        _keyBoxes[prop] = tb;
    }

    void StyleBtnNoDock(Control b, string text, Color bg)
    {
        b.Text = text;
        b.BackColor = bg;
        b.ForeColor = TextPri;
        b.Height = 36;
        b.Margin = new Padding(0, 0, 12, 0);
        if (b is Button btn)
        {
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderSize = 0;
            btn.TextAlign = ContentAlignment.MiddleCenter;
            btn.Cursor = Cursors.Hand;
            btn.Dock = DockStyle.None;
        }
    }

    void ApplyKeyBox(string prop, string text)
    {
        if (_settings.KeyMap == null) _settings.KeyMap = KeyMap.Defaults();
        var pi = typeof(KeyMap).GetProperty(prop);
        if (pi != null && pi.CanWrite) pi.SetValue(_settings.KeyMap, (text ?? "").Trim());
    }

    void HideControllerAfterBuild()
    {
        if (_controllerHiddenAfterBuild) return;
        _controllerHiddenAfterBuild = true;
        try
        {
            if (_controllerTab != null && _tabs.TabPages.Contains(_controllerTab))
            {
                if (_tabs.SelectedTab == _controllerTab) _tabs.SelectedIndex = 0;
                _tabs.TabPages.Remove(_controllerTab);
            }
            _controllerMapped = true;
            Append("Controller tab closed — mapping locked for this build.");
        }
        catch { }
    }

    void WritePadMapForRuntime()
    {
        try
        {
            if (_settings.KeyMap == null) _settings.KeyMap = KeyMap.Defaults();
            string path = Path.Combine(Settings.DataDir, "pad_map.json");
            File.WriteAllText(path, JsonSerializer.Serialize(_settings.KeyMap, new JsonSerializerOptions { WriteIndented = true }));
            if (!string.IsNullOrEmpty(_projectDir))
            {
                string outDir = Path.Combine(_projectDir, "output");
                try { Directory.CreateDirectory(outDir); } catch { }
                File.WriteAllText(Path.Combine(outDir, "pad_map.json"), File.ReadAllText(path));
            }
        }
        catch { }
    }
}
