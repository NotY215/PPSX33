using System.Reflection;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private readonly Dictionary<string, Button> _keyButtons = new();

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(8, 8, 10),
            AutoScroll = true,
            Padding = new Padding(12)
        };

        var root = new Panel
        {
            Dock = DockStyle.Top,
            Height = 520,
            BackColor = Color.FromArgb(8, 8, 10)
        };

        // Center controller art
        var art = new PictureBox
        {
            Size = new Size(360, 280),
            Location = new Point(200, 100),
            SizeMode = PictureBoxSizeMode.Zoom,
            BackColor = Color.Transparent
        };
        try
        {
            foreach (var p in new[]
            {
                Path.Combine(AppContext.BaseDirectory, "assets", "PPSX33 PS3 Controller Key Mapping.png"),
                Path.Combine(AppContext.BaseDirectory, "PPSX33 PS3 Controller Key Mapping.png"),
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.png"),
            })
            {
                if (!File.Exists(p)) continue;
                art.Image = Image.FromFile(p);
                break;
            }
        }
        catch { }
        root.Controls.Add(art);

        var km = _settings.Keys ?? KeyMap.Defaults();

        // Helper local factory for binding chips
        Button Chip(string prop, string caption, string value, int x, int y, int w = 88)
        {
            var outer = new Panel
            {
                Location = new Point(x, y),
                Size = new Size(w, 44),
                BackColor = Color.FromArgb(20, 20, 24)
            };
            outer.Paint += (_, e) =>
            {
                using var pen = new Pen(Color.FromArgb(70, 70, 80));
                e.Graphics.DrawRectangle(pen, 0, 0, outer.Width - 1, outer.Height - 1);
            };
            var lab = new Label
            {
                Text = caption,
                ForeColor = Color.FromArgb(160, 160, 170),
                Font = new Font("Segoe UI", 7f),
                Location = new Point(4, 2),
                AutoSize = true,
                BackColor = Color.Transparent
            };
            var btn = new Button
            {
                Text = PrettyKey(value),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(28, 28, 34),
                ForeColor = Color.FromArgb(230, 230, 235),
                Font = new Font("Consolas", 8.5f, FontStyle.Bold),
                Location = new Point(6, 16),
                Size = new Size(w - 12, 22),
                Cursor = Cursors.Hand,
                Tag = prop
            };
            btn.FlatAppearance.BorderColor = Color.FromArgb(90, 90, 100);
            btn.FlatAppearance.BorderSize = 1;
            btn.Click += KeyBind_Click;
            outer.Controls.Add(lab);
            outer.Controls.Add(btn);
            root.Controls.Add(outer);
            _keyButtons[prop] = btn;
            return btn;
        }

        // D-Pad group (left)
        GroupBox("D-Pad", 12, 12, 150, 150);
        Chip(nameof(KeyMap.DpadUp), "Up", km.DpadUp, 48, 36, 78);
        Chip(nameof(KeyMap.DpadLeft), "Left", km.DpadLeft, 16, 80, 70);
        Chip(nameof(KeyMap.DpadRight), "Right", km.DpadRight, 90, 80, 70);
        Chip(nameof(KeyMap.DpadDown), "Down", km.DpadDown, 48, 120, 78);

        // Left stick
        GroupBox("Left Stick", 12, 180, 150, 150);
        Chip(nameof(KeyMap.LeftStickUp), "Up", km.LeftStickUp, 48, 204, 78);
        Chip(nameof(KeyMap.LeftStickLeft), "Left", km.LeftStickLeft, 16, 248, 70);
        Chip(nameof(KeyMap.LeftStickRight), "Right", km.LeftStickRight, 90, 248, 70);
        Chip(nameof(KeyMap.LeftStickDown), "Down", km.LeftStickDown, 48, 288, 78);

        // Shoulders top
        Chip(nameof(KeyMap.L1), "L1", km.L1, 180, 16, 70);
        Chip(nameof(KeyMap.L2), "L2", km.L2, 180, 64, 90);
        Chip(nameof(KeyMap.R1), "R1", km.R1, 520, 16, 70);
        Chip(nameof(KeyMap.R2), "R2", km.R2, 500, 64, 90);

        // Select / Start / PS
        Chip(nameof(KeyMap.Select), "Select", km.Select, 280, 20, 80);
        Chip(nameof(KeyMap.Start), "Start", km.Start, 370, 20, 70);
        Chip(nameof(KeyMap.PsButton), "PS Button", km.PsButton, 320, 64, 100);

        // Face buttons
        GroupBox("Face Buttons", 580, 12, 160, 160);
        Chip(nameof(KeyMap.Triangle), "Triangle", km.Triangle, 620, 36, 80);
        Chip(nameof(KeyMap.Square), "Square", km.Square, 590, 80, 70);
        Chip(nameof(KeyMap.Circle), "Circle", km.Circle, 670, 80, 70);
        Chip(nameof(KeyMap.Cross), "Cross", km.Cross, 620, 124, 80);

        // Right stick
        GroupBox("Right Stick", 580, 180, 160, 150);
        Chip(nameof(KeyMap.RightStickUp), "Up", km.RightStickUp, 630, 204, 70);
        Chip(nameof(KeyMap.RightStickLeft), "Left", km.RightStickLeft, 590, 248, 70);
        Chip(nameof(KeyMap.RightStickRight), "Right", km.RightStickRight, 670, 248, 70);
        Chip(nameof(KeyMap.RightStickDown), "Down", km.RightStickDown, 630, 288, 70);

        // L3 / R3 under pad
        Chip(nameof(KeyMap.L3), "L3", km.L3, 280, 400, 90);
        Chip(nameof(KeyMap.R3), "R3", km.R3, 400, 400, 90);

        var save = new Button
        {
            Text = "Save mapping",
            Location = new Point(16, 460),
            Size = new Size(120, 28),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(31, 111, 235),
            ForeColor = Color.White
        };
        save.Click += (_, _) =>
        {
            SaveKeyMapFromButtons();
            _controllerMapped = true;
            Append("Controller map saved.");
            UpdateButtons();
        };
        var reset = new Button
        {
            Text = "Reset defaults",
            Location = new Point(150, 460),
            Size = new Size(120, 28),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(40, 40, 48),
            ForeColor = Color.White
        };
        reset.Click += (_, _) =>
        {
            _settings.Keys = KeyMap.Defaults();
            SaveSettings();
            foreach (var kv in _keyButtons)
            {
                var prop = kv.Key;
                var val = typeof(KeyMap).GetProperty(prop)?.GetValue(_settings.Keys)?.ToString() ?? "";
                kv.Value.Text = PrettyKey(val);
            }
            Append("Controller map reset to defaults.");
        };
        root.Controls.Add(save);
        root.Controls.Add(reset);

        void GroupBox(string title, int x, int y, int w, int h)
        {
            var g = new Label
            {
                Text = title,
                ForeColor = Color.FromArgb(140, 140, 150),
                Font = new Font("Segoe UI", 8f, FontStyle.Bold),
                Location = new Point(x, y),
                AutoSize = true,
                BackColor = Color.Transparent
            };
            var border = new Panel
            {
                Location = new Point(x, y + 16),
                Size = new Size(w, h - 16),
                BackColor = Color.FromArgb(14, 14, 18)
            };
            border.Paint += (_, e) =>
            {
                using var pen = new Pen(Color.FromArgb(50, 50, 58));
                e.Graphics.DrawRectangle(pen, 0, 0, border.Width - 1, border.Height - 1);
            };
            root.Controls.Add(border);
            root.Controls.Add(g);
            g.BringToFront();
        }

        _controllerPanel.Controls.Add(root);
        _controllerTab = new TabPage("Controller")
        {
            BackColor = Color.FromArgb(8, 8, 10),
            Padding = new Padding(4)
        };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    static string PrettyKey(string k)
    {
        if (string.IsNullOrEmpty(k)) return "—";
        return k
            .Replace("NumPad", "Num+")
            .Replace("RControlKey", "Ctrl Right")
            .Replace("LControlKey", "Ctrl Left")
            .Replace("RShiftKey", "Shift Right")
            .Replace("LShiftKey", "Shift Left")
            .Replace("ControlKey", "Ctrl")
            .Replace("ShiftKey", "Shift");
    }

    void KeyBind_Click(object? sender, EventArgs e)
    {
        if (sender is not Button btn || btn.Tag is not string prop) return;
        btn.Text = "Press key…";
        btn.BackColor = Color.FromArgb(40, 60, 40);
        void OnKey(object? s, KeyEventArgs ke)
        {
            ke.Handled = true;
            ke.SuppressKeyPress = true;
            string name = ke.KeyCode.ToString();
            if (ke.KeyCode == Keys.ShiftKey) name = ke.KeyValue == 0xA1 ? "RShiftKey" : "LShiftKey";
            if (ke.KeyCode == Keys.ControlKey) name = ke.KeyValue == 0xA3 ? "RControlKey" : "LControlKey";
            btn.Text = PrettyKey(name);
            btn.Tag = prop + "|" + name;
            btn.BackColor = Color.FromArgb(28, 28, 34);
            KeyUp -= OnKey;
        }
        KeyPreview = true;
        KeyUp += OnKey;
    }

    void SaveKeyMapFromButtons()
    {
        var km = _settings.Keys ?? KeyMap.Defaults();
        foreach (var kv in _keyButtons)
        {
            string prop = kv.Key;
            string text = kv.Value.Tag is string t && t.Contains('|') ? t.Split('|')[1] : InversePretty(kv.Value.Text);
            typeof(KeyMap).GetProperty(prop)?.SetValue(km, text);
        }
        _settings.Keys = km;
        SaveSettings();
        _controllerMapped = true;
    }

    static string InversePretty(string pretty)
    {
        return pretty
            .Replace("Num+", "NumPad")
            .Replace("Ctrl Right", "RControlKey")
            .Replace("Ctrl Left", "LControlKey")
            .Replace("Shift Right", "RShiftKey")
            .Replace("Shift Left", "LShiftKey");
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
