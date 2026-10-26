using System.Drawing.Drawing2D;
using System.Text.Json;

namespace PS3Recomp.Gui;

public sealed partial class MainForm
{
    private Panel? _controllerPanel;
    private TabPage? _controllerTab;
    private DualShockMapControl? _dsMap;
    private readonly Dictionary<string, string> _bind = new();

    void BuildControllerTab()
    {
        if (_controllerTab != null) return;

        _controllerPanel = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(9, 13, 22),
            Padding = new Padding(8)
        };

        // Load keymap into bind dict
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        void Put(string id, string v) => _bind[id] = v;
        Put("dpad_up", km.DpadUp); Put("dpad_down", km.DpadDown);
        Put("dpad_left", km.DpadLeft); Put("dpad_right", km.DpadRight);
        Put("triangle", km.Triangle); Put("circle", km.Circle);
        Put("cross", km.Cross); Put("square", km.Square);
        Put("l1", km.L1); Put("r1", km.R1); Put("l2", km.L2); Put("r2", km.R2);
        Put("select", km.Select); Put("start", km.Start); Put("ps", km.PsButton);
        Put("ls_up", km.LeftStickUp); Put("ls_down", km.LeftStickDown);
        Put("ls_left", km.LeftStickLeft); Put("ls_right", km.LeftStickRight);
        Put("rs_up", km.RightStickUp); Put("rs_down", km.RightStickDown);
        Put("rs_left", km.RightStickLeft); Put("rs_right", km.RightStickRight);
        Put("l3", km.L3); Put("r3", km.R3);

        var topBar = new Panel
        {
            Dock = DockStyle.Top,
            Height = 44,
            BackColor = Color.FromArgb(15, 20, 32)
        };
        var presetLbl = new Label
        {
            Text = "Preset",
            ForeColor = Color.FromArgb(148, 163, 184),
            Location = new Point(12, 12),
            AutoSize = true
        };
        var preset = new ComboBox
        {
            DropDownStyle = ComboBoxStyle.DropDownList,
            Location = new Point(60, 8),
            Width = 200,
            BackColor = Color.FromArgb(15, 23, 42),
            ForeColor = Color.FromArgb(251, 113, 133)
        };
        preset.Items.AddRange(new object[] {
            "RPCS3 / PPSX33 Default", "FPS / Shooter", "Fighting (Arcade)", "Racing", "Custom"
        });
        preset.SelectedIndex = 0;
        preset.SelectedIndexChanged += (_, _) =>
        {
            ApplyPreset(preset.SelectedIndex);
            _dsMap?.SetBinds(_bind);
            _dsMap?.Invalidate();
        };

        var btnSave = MakeTopBtn("Save mapping", Color.FromArgb(31, 111, 235), 280);
        btnSave.Click += (_, _) =>
        {
            SaveBindsToSettings();
            WritePadMapForRuntime();
            _controllerMapped = true;
            Append("Controller mapping saved.");
            UpdateButtons();
        };
        var btnReset = MakeTopBtn("Reset defaults", Color.FromArgb(40, 40, 48), 400);
        btnReset.Click += (_, _) =>
        {
            preset.SelectedIndex = 0;
            ApplyPreset(0);
            _dsMap?.SetBinds(_bind);
            _dsMap?.Invalidate();
            Append("Controller reset to defaults.");
        };
        topBar.Controls.Add(presetLbl);
        topBar.Controls.Add(preset);
        topBar.Controls.Add(btnSave);
        topBar.Controls.Add(btnReset);

        _dsMap = new DualShockMapControl
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(9, 13, 22)
        };
        _dsMap.SetBinds(_bind);
        _dsMap.BindRequested += id =>
        {
            string? key = CaptureKeyDialog(id);
            if (key == null) return;
            _bind[id] = key;
            _dsMap.SetBinds(_bind);
            _dsMap.Invalidate();
            if (preset.SelectedIndex != 4) preset.SelectedIndex = 4;
        };

        _controllerPanel.Controls.Add(_dsMap);
        _controllerPanel.Controls.Add(topBar);

        _controllerTab = new TabPage("Controller")
        {
            BackColor = Color.FromArgb(9, 13, 22),
            Padding = new Padding(0)
        };
        _controllerTab.Controls.Add(_controllerPanel);
        _tabs.TabPages.Add(_controllerTab);
    }

    static Button MakeTopBtn(string text, Color bg, int x)
    {
        var b = new Button
        {
            Text = text,
            Location = new Point(x, 8),
            Size = new Size(110, 28),
            FlatStyle = FlatStyle.Flat,
            BackColor = bg,
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 8.5f, FontStyle.Bold)
        };
        b.FlatAppearance.BorderSize = 0;
        return b;
    }

    void ApplyPreset(int idx)
    {
        // 0 = defaults from KeyMap
        var d = KeyMap.Defaults();
        void All(KeyMap k)
        {
            _bind["dpad_up"] = k.DpadUp; _bind["dpad_down"] = k.DpadDown;
            _bind["dpad_left"] = k.DpadLeft; _bind["dpad_right"] = k.DpadRight;
            _bind["triangle"] = k.Triangle; _bind["circle"] = k.Circle;
            _bind["cross"] = k.Cross; _bind["square"] = k.Square;
            _bind["l1"] = k.L1; _bind["r1"] = k.R1; _bind["l2"] = k.L2; _bind["r2"] = k.R2;
            _bind["select"] = k.Select; _bind["start"] = k.Start; _bind["ps"] = k.PsButton;
            _bind["ls_up"] = k.LeftStickUp; _bind["ls_down"] = k.LeftStickDown;
            _bind["ls_left"] = k.LeftStickLeft; _bind["ls_right"] = k.LeftStickRight;
            _bind["rs_up"] = k.RightStickUp; _bind["rs_down"] = k.RightStickDown;
            _bind["rs_left"] = k.RightStickLeft; _bind["rs_right"] = k.RightStickRight;
            _bind["l3"] = k.L3; _bind["r3"] = k.R3;
        }
        if (idx == 0) { All(d); return; }
        if (idx == 1) // FPS
        {
            All(d);
            _bind["ls_up"] = "W"; _bind["ls_down"] = "S"; _bind["ls_left"] = "A"; _bind["ls_right"] = "D";
            _bind["cross"] = "Space"; _bind["square"] = "R"; _bind["r2"] = "MouseLeft"; _bind["l2"] = "MouseRight";
            return;
        }
        if (idx == 2) // Fighting
        {
            All(d);
            _bind["dpad_up"] = "W"; _bind["dpad_down"] = "S"; _bind["dpad_left"] = "A"; _bind["dpad_right"] = "D";
            _bind["square"] = "U"; _bind["triangle"] = "I"; _bind["cross"] = "J"; _bind["circle"] = "K";
            return;
        }
        if (idx == 3) // Racing
        {
            All(d);
            _bind["cross"] = "W"; _bind["square"] = "S"; _bind["l2"] = "LControlKey"; _bind["r2"] = "LShiftKey";
            return;
        }
    }

    void SaveBindsToSettings()
    {
        var km = _settings.KeyMap ?? KeyMap.Defaults();
        km.DpadUp = G("dpad_up"); km.DpadDown = G("dpad_down");
        km.DpadLeft = G("dpad_left"); km.DpadRight = G("dpad_right");
        km.Triangle = G("triangle"); km.Circle = G("circle");
        km.Cross = G("cross"); km.Square = G("square");
        km.L1 = G("l1"); km.R1 = G("r1"); km.L2 = G("l2"); km.R2 = G("r2");
        km.Select = G("select"); km.Start = G("start"); km.PsButton = G("ps");
        km.LeftStickUp = G("ls_up"); km.LeftStickDown = G("ls_down");
        km.LeftStickLeft = G("ls_left"); km.LeftStickRight = G("ls_right");
        km.RightStickUp = G("rs_up"); km.RightStickDown = G("rs_down");
        km.RightStickLeft = G("rs_left"); km.RightStickRight = G("rs_right");
        km.L3 = G("l3"); km.R3 = G("r3");
        _settings.KeyMap = km;
        SaveSettings();
        string G(string id) => _bind.TryGetValue(id, out var v) ? v : "";
    }

    string? CaptureKeyDialog(string buttonId)
    {
        using var f = new Form
        {
            Text = "Remap — " + buttonId,
            Size = new Size(360, 160),
            StartPosition = FormStartPosition.CenterParent,
            FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false,
            MinimizeBox = false,
            BackColor = Color.FromArgb(15, 23, 42),
            ForeColor = Color.White,
            KeyPreview = true
        };
        var lab = new Label
        {
            Text = "Press a key for " + buttonId + "\n(Esc to cancel)",
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleCenter,
            Font = new Font("Segoe UI", 11f, FontStyle.Bold)
        };
        f.Controls.Add(lab);
        string? result = null;
        f.KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Escape) { f.DialogResult = DialogResult.Cancel; f.Close(); return; }
            result = e.KeyCode.ToString();
            if (e.KeyCode == Keys.ShiftKey) result = Control.ModifierKeys.HasFlag(Keys.RShiftKey) ? "RShiftKey" : "LShiftKey";
            if (e.KeyCode == Keys.ControlKey) result = "LControlKey";
            f.DialogResult = DialogResult.OK;
            f.Close();
        };
        return f.ShowDialog(this) == DialogResult.OK ? result : null;
    }

    void WritePadMapForRuntime()
    {
        try
        {
            if (string.IsNullOrEmpty(_projectDir)) return;
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

/// <summary>Interactive DualShock 3 diagram + callout badges (WinForms GDI+).</summary>
internal sealed class DualShockMapControl : Control
{
    readonly Dictionary<string, string> _binds = new();
    readonly Dictionary<string, RectangleF> _hot = new();
    string? _hover;
    string? _pulseId;
    float _pulse;
    readonly System.Windows.Forms.Timer _anim = new() { Interval = 33 };

    public event Action<string>? BindRequested;

    public DualShockMapControl()
    {
        DoubleBuffered = true;
        SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
        _anim.Tick += (_, _) =>
        {
            _pulse += 0.12f;
            if (_pulse > MathF.PI * 2) _pulse = 0;
            Invalidate();
        };
        _anim.Start();
        MouseMove += (_, e) =>
        {
            string? h = Hit(e.Location);
            if (h != _hover) { _hover = h; Cursor = h != null ? Cursors.Hand : Cursors.Default; Invalidate(); }
        };
        MouseClick += (_, e) =>
        {
            string? id = Hit(e.Location);
            if (id != null) BindRequested?.Invoke(id);
        };
        Disposed += (_, _) => _anim.Stop();
    }

    public void SetBinds(Dictionary<string, string> src)
    {
        _binds.Clear();
        foreach (var kv in src) _binds[kv.Key] = Pretty(kv.Value);
    }

    static string Pretty(string k) => string.IsNullOrEmpty(k) ? "—" : k
        .Replace("NumPad", "Num+")
        .Replace("RControlKey", "Ctrl R")
        .Replace("LControlKey", "Ctrl L")
        .Replace("RShiftKey", "Shift R")
        .Replace("LShiftKey", "Shift L")
        .Replace("ControlKey", "Ctrl")
        .Replace("ShiftKey", "Shift");

    string? Hit(Point p)
    {
        foreach (var kv in _hot)
            if (kv.Value.Contains(p)) return kv.Key;
        return null;
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        var g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.Clear(BackColor);
        _hot.Clear();

        float cx = Width / 2f;
        float cy = Height / 2f + 10;
        float s = Math.Min(Width / 900f, Height / 560f);
        if (s < 0.55f) s = 0.55f;

        // Card background
        using (var path = RoundRect(new RectangleF(8, 8, Width - 16, Height - 16), 16))
        using (var br = new SolidBrush(Color.FromArgb(19, 27, 46)))
        using (var pen = new Pen(Color.FromArgb(40, 50, 70)))
        {
            g.FillPath(br, path);
            g.DrawPath(pen, path);
        }

        // Header
        using (var f = new Font("Segoe UI", 10f, FontStyle.Bold))
        using (var br = new SolidBrush(Color.White))
            g.DrawString("Interactive DualShock 3  ·  click a badge or pad area to remap", f, br, 24, 18);

        // Controller shell
        float ox = cx - 280 * s;
        float oy = cy - 160 * s;
        var shell = new GraphicsPath();
        shell.AddBezier(ox + 40*s, oy + 40*s, ox + 120*s, oy + 10*s, ox + 240*s, oy + 20*s, ox + 280*s, oy + 30*s);
        shell.AddBezier(ox + 320*s, oy + 20*s, ox + 440*s, oy + 10*s, ox + 520*s, oy + 40*s, ox + 540*s, oy + 100*s);
        shell.AddBezier(ox + 560*s, oy + 180*s, ox + 540*s, oy + 260*s, ox + 480*s, oy + 280*s, ox + 420*s, oy + 250*s);
        shell.AddBezier(ox + 360*s, oy + 230*s, ox + 300*s, oy + 230*s, ox + 240*s, oy + 230*s, ox + 180*s, oy + 250*s);
        shell.AddBezier(ox + 100*s, oy + 280*s, ox + 40*s, oy + 260*s, ox + 20*s, oy + 180*s, ox + 40*s, oy + 100*s);
        shell.CloseFigure();

        using (var br = new LinearGradientBrush(new RectangleF(ox, oy, 560*s, 300*s),
                   Color.FromArgb(55, 60, 75), Color.FromArgb(18, 20, 28), 90f))
        using (var pen = new Pen(Color.FromArgb(80, 90, 110), 2.5f * s))
        {
            g.FillPath(br, shell);
            g.DrawPath(pen, shell);
        }

        // Title on shell
        using (var f = new Font("Segoe UI", 9f * s, FontStyle.Bold))
        using (var br = new SolidBrush(Color.FromArgb(120, 130, 150)))
        {
            var sz = g.MeasureString("DUALSHOCK 3", f);
            g.DrawString("DUALSHOCK 3", f, br, cx - sz.Width / 2, oy + 50 * s);
        }

        // D-pad cross
        DrawDpad(g, ox + 120 * s, oy + 140 * s, 36 * s);
        // Face buttons
        DrawFace(g, ox + 420 * s, oy + 140 * s, 36 * s);
        // Sticks
        DrawStick(g, ox + 180 * s, oy + 210 * s, 28 * s, "L");
        DrawStick(g, ox + 360 * s, oy + 210 * s, 28 * s, "R");
        // Shoulders
        DrawShoulder(g, ox + 100 * s, oy + 20 * s, 70 * s, 22 * s, "L1");
        DrawShoulder(g, ox + 100 * s, oy - 8 * s, 70 * s, 18 * s, "L2");
        DrawShoulder(g, ox + 390 * s, oy + 20 * s, 70 * s, 22 * s, "R1");
        DrawShoulder(g, ox + 390 * s, oy - 8 * s, 70 * s, 18 * s, "R2");
        // Select / Start / PS
        DrawSmall(g, ox + 230 * s, oy + 120 * s, "SELECT");
        DrawSmall(g, ox + 300 * s, oy + 120 * s, "START");
        DrawPs(g, cx, oy + 150 * s, 14 * s);

        // Callout badges around controller
        Badge(g, "L2", "l2", ox + 20 * s, oy - 40 * s);
        Badge(g, "L1", "l1", ox + 20 * s, oy - 5 * s);
        Badge(g, "R2", "r2", ox + 500 * s, oy - 40 * s);
        Badge(g, "R1", "r1", ox + 500 * s, oy - 5 * s);

        Badge(g, "Up", "dpad_up", ox - 10 * s, oy + 90 * s);
        Badge(g, "Left", "dpad_left", ox - 30 * s, oy + 130 * s);
        Badge(g, "Right", "dpad_right", ox + 90 * s, oy + 130 * s);
        Badge(g, "Down", "dpad_down", ox - 10 * s, oy + 170 * s);

        Badge(g, "△", "triangle", ox + 500 * s, oy + 90 * s);
        Badge(g, "□", "square", ox + 480 * s, oy + 125 * s);
        Badge(g, "○", "circle", ox + 530 * s, oy + 125 * s);
        Badge(g, "✕", "cross", ox + 500 * s, oy + 160 * s);

        Badge(g, "LS ↑", "ls_up", ox + 80 * s, oy + 250 * s);
        Badge(g, "LS ←", "ls_left", ox + 40 * s, oy + 280 * s);
        Badge(g, "LS →", "ls_right", ox + 130 * s, oy + 280 * s);
        Badge(g, "LS ↓", "ls_down", ox + 80 * s, oy + 310 * s);
        Badge(g, "L3", "l3", ox + 200 * s, oy + 300 * s);

        Badge(g, "RS ↑", "rs_up", ox + 360 * s, oy + 250 * s);
        Badge(g, "RS ←", "rs_left", ox + 320 * s, oy + 280 * s);
        Badge(g, "RS →", "rs_right", ox + 410 * s, oy + 280 * s);
        Badge(g, "RS ↓", "rs_down", ox + 360 * s, oy + 310 * s);
        Badge(g, "R3", "r3", ox + 280 * s, oy + 300 * s);

        Badge(g, "Select", "select", ox + 210 * s, oy + 70 * s);
        Badge(g, "Start", "start", ox + 300 * s, oy + 70 * s);
        Badge(g, "PS", "ps", cx - 20 * s, oy + 175 * s);

        using (var f = new Font("Segoe UI", 8f))
        using (var br = new SolidBrush(Color.FromArgb(100, 116, 139)))
            g.DrawString("Live: click badge · pulse = hover  ·  mappings saved to pad_map.json on Save", f, br, 24, Height - 28);
    }

    void Badge(Graphics g, string label, string id, float x, float y)
    {
        string key = _binds.TryGetValue(id, out var v) ? v : "—";
        string text = label + "  " + key;
        using var font = new Font("Consolas", 8f, FontStyle.Bold);
        var sz = g.MeasureString(text, font);
        var rect = new RectangleF(x, y, Math.Max(sz.Width + 14, 56), 22);
        _hot[id] = rect;
        bool hot = _hover == id;
        float glow = hot ? (0.5f + 0.5f * MathF.Sin(_pulse)) : 0;
        Color border = hot ? Color.FromArgb(239, 68, 68) : Color.FromArgb(56, 189, 248);
        Color fill = hot ? Color.FromArgb(60, 239, 68, 68) : Color.FromArgb(30, 41, 59);
        using (var path = RoundRect(rect, 6))
        using (var br = new SolidBrush(fill))
        using (var pen = new Pen(Color.FromArgb((int)(180 + 75 * glow), border), 1.5f + glow))
        {
            g.FillPath(br, path);
            g.DrawPath(pen, path);
        }
        using (var br = new SolidBrush(Color.White))
            g.DrawString(text, font, br, rect.X + 6, rect.Y + 3);
    }

    static void DrawDpad(Graphics g, float cx, float cy, float r)
    {
        using var br = new SolidBrush(Color.FromArgb(30, 34, 44));
        using var pen = new Pen(Color.FromArgb(70, 80, 100), 2);
        g.FillEllipse(br, cx - r, cy - r, r * 2, r * 2);
        g.DrawEllipse(pen, cx - r, cy - r, r * 2, r * 2);
        using var arm = new SolidBrush(Color.FromArgb(50, 55, 70));
        g.FillRectangle(arm, cx - 8, cy - r + 6, 16, r * 2 - 12);
        g.FillRectangle(arm, cx - r + 6, cy - 8, r * 2 - 12, 16);
    }

    static void DrawFace(Graphics g, float cx, float cy, float r)
    {
        using var br = new SolidBrush(Color.FromArgb(30, 34, 44));
        using var pen = new Pen(Color.FromArgb(70, 80, 100), 2);
        g.FillEllipse(br, cx - r, cy - r, r * 2, r * 2);
        g.DrawEllipse(pen, cx - r, cy - r, r * 2, r * 2);
        void Dot(float x, float y, Color c)
        {
            using var b = new SolidBrush(Color.FromArgb(40, 44, 56));
            using var p = new Pen(c, 2);
            g.FillEllipse(b, x - 10, y - 10, 20, 20);
            g.DrawEllipse(p, x - 10, y - 10, 20, 20);
        }
        Dot(cx, cy - 22, Color.FromArgb(16, 185, 129)); // triangle
        Dot(cx + 22, cy, Color.FromArgb(239, 68, 68));   // circle
        Dot(cx, cy + 22, Color.FromArgb(59, 130, 246));  // cross
        Dot(cx - 22, cy, Color.FromArgb(236, 72, 153));  // square
    }

    static void DrawStick(Graphics g, float cx, float cy, float r, string tag)
    {
        using var br = new LinearGradientBrush(new RectangleF(cx - r, cy - r, r * 2, r * 2),
            Color.FromArgb(40, 44, 56), Color.FromArgb(20, 22, 30), 90f);
        using var pen = new Pen(Color.FromArgb(90, 100, 120), 2);
        g.FillEllipse(br, cx - r, cy - r, r * 2, r * 2);
        g.DrawEllipse(pen, cx - r, cy - r, r * 2, r * 2);
        using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
        using var tb = new SolidBrush(Color.FromArgb(140, 150, 170));
        g.DrawString(tag, f, tb, cx - 5, cy - 6);
    }

    static void DrawShoulder(Graphics g, float x, float y, float w, float h, string t)
    {
        using var br = new SolidBrush(Color.FromArgb(45, 50, 64));
        using var pen = new Pen(Color.FromArgb(80, 90, 110));
        g.FillRectangle(br, x, y, w, h);
        g.DrawRectangle(pen, x, y, w, h);
        using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
        using var tb = new SolidBrush(Color.FromArgb(180, 190, 200));
        g.DrawString(t, f, tb, x + 8, y + 2);
    }

    static void DrawSmall(Graphics g, float x, float y, string t)
    {
        using var f = new Font("Segoe UI", 6.5f, FontStyle.Bold);
        using var br = new SolidBrush(Color.FromArgb(100, 110, 130));
        g.DrawString(t, f, br, x, y);
    }

    static void DrawPs(Graphics g, float cx, float cy, float r)
    {
        using var br = new SolidBrush(Color.FromArgb(30, 34, 44));
        using var pen = new Pen(Color.FromArgb(200, 200, 210), 1.5f);
        g.FillEllipse(br, cx - r, cy - r, r * 2, r * 2);
        g.DrawEllipse(pen, cx - r, cy - r, r * 2, r * 2);
    }

    static GraphicsPath RoundRect(RectangleF r, float radius)
    {
        float d = radius * 2;
        var p = new GraphicsPath();
        p.AddArc(r.X, r.Y, d, d, 180, 90);
        p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
        p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        p.CloseFigure();
        return p;
    }
}
