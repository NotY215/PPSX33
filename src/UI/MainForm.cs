using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;

namespace PS3Recomp.Gui;

public sealed partial class MainForm : Form
{
    // Website palette
    static readonly Color Bg = Color.FromArgb(0x09, 0x0D, 0x14);
    static readonly Color Panel = Color.FromArgb(0x0D, 0x11, 0x17);
    static readonly Color Panel2 = Color.FromArgb(0x16, 0x1B, 0x22);
    static readonly Color BorderC = Color.FromArgb(0x30, 0x36, 0x3D);
    static readonly Color Hover = Color.FromArgb(0x21, 0x26, 0x2D);
    static readonly Color TextPri = Color.FromArgb(0xE6, 0xED, 0xF3);
    static readonly Color TextSec = Color.FromArgb(0x8B, 0x94, 0x9E);
    static readonly Color Accent = Color.FromArgb(0x38, 0xBD, 0xF8);
    static readonly Color AccentBlue = Color.FromArgb(0x1F, 0x6F, 0xEB);
    static readonly Color Success = Color.FromArgb(0x23, 0x86, 0x36);
    static readonly Color Danger = Color.FromArgb(0xDA, 0x36, 0x33);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly LegalTab _legalTab = new();
    private readonly TextBox _roadmap = new();
    private readonly TextBox _requirements = new();
    private readonly Ps3HudLabel _project = new();
    private readonly Label _status = new();
    private readonly ProgressBar _progress = new();
    private readonly Ps3Button _btnLoad = new();
    private readonly Ps3Button _btnLift = new();
    private readonly Ps3Button _btnBuild = new();
    private readonly Ps3Button _btnRun = new();
    private readonly Ps3Button _btnCopy = new();
    private readonly Ps3Button _btnCancel = new();
    private readonly ComboBox _gfx = new();
    private readonly ComboBox _threads = new();
    private readonly TabControl _tabs = new();
    private readonly PictureBox _logo = new();
    private readonly Label _eta = new();
    private Label? _headerStatus;
    private Panel? _logoRingPanel;
    private System.Windows.Forms.Timer? _animTimer;
    private float _ringAngle;
    private Font? _logoFontTitle;
    private Font? _logoFontSub;
    private Pen? _logoPenDash;
    private Pen? _logoPenArc;

    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _building;
    private bool _buildCancelRequested;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private bool _controllerMapped;
    private bool _controllerHiddenAfterBuild;
    /// <summary>True after user saves controller map or build locks mapping.</summary>
    internal bool ControllerMapped => _controllerMapped;

    // Headers under runtime/; ps3rt.dll + vulkan-1.dll live next to the EXE (BaseDirectory)
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "redist");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    protected override CreateParams CreateParams
    {
        get
        {
            var cp = base.CreateParams;
            cp.ExStyle |= 0x02000000; // WS_EX_COMPOSITED — smoother paint
            return cp;
        }
    }

    public MainForm()
    {
        Text = "PPSX33 · PlayStation 3 Recompiler";
        StartPosition = FormStartPosition.CenterScreen;
        Size = new Size(1280, 800);
        MinimumSize = new Size(1024, 640);
        BackColor = Bg;
        ForeColor = TextPri;
        Font = new Font("Segoe UI", 9f);
        DoubleBuffered = true;
        SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer, true);

        try
        {
            Load += (_, _) =>
            {
                int useDark = 1;
                DwmSetWindowAttribute(Handle, 20, ref useDark, sizeof(int));
            };
        }
        catch { }

        // Window + taskbar icon from built-in .ico only (no separate .png copy)
        try
        {
            foreach (var ico in new[]
            {
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico"),
            })
            {
                if (File.Exists(ico)) { Icon = new Icon(ico); break; }
            }
        }
        catch { }

        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            BackColor = Bg,
            Padding = Padding.Empty
        };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 288));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));

        // ===== SIDEBAR =====
        var sidebar = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Panel,
            Padding = new Padding(14),
            AutoScroll = true
        };
        sidebar.Paint += (_, e) =>
        {
            using var pen = new Pen(BorderC);
            e.Graphics.DrawLine(pen, sidebar.ClientSize.Width - 1, 0, sidebar.ClientSize.Width - 1, sidebar.ClientSize.Height);
        };

        var rail = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            ColumnCount = 1,
            BackColor = Panel
        };
        rail.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));

        // Logo card with PNG + animated ring
        _logoRingPanel = new Panel
        {
            Height = 132,
            Dock = DockStyle.Top,
            BackColor = Panel2,
            Margin = new Padding(0, 0, 0, 12)
        };
        _logoRingPanel.Paint += PaintLogoCard;
        // Double-buffer ring panel (stops logo animation shuttering)
        typeof(Control).InvokeMember("DoubleBuffered",
            System.Reflection.BindingFlags.SetProperty | System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic,
            null, _logoRingPanel, new object[] { true });
        _logoFontTitle = new Font("Segoe UI", 13f, FontStyle.Bold);
        _logoFontSub = new Font("Segoe UI", 7f, FontStyle.Bold);
        _logoPenDash = new Pen(Color.FromArgb(140, Accent), 1.5f) { DashStyle = DashStyle.Dash };
        _logoPenArc = new Pen(Color.FromArgb(160, AccentBlue), 1.5f);
        _logo.Size = new Size(56, 56);
        _logo.SizeMode = PictureBoxSizeMode.Zoom;
        _logo.BackColor = Color.Transparent;
        // UI logo asset: load from the same in-build .ico (Bitmap), not a second .png file
        try
        {
            foreach (var ico in new[]
            {
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico"),
            })
            {
                if (!File.Exists(ico)) continue;
                using var icon = new Icon(ico, 64, 64);
                _logo.Image = icon.ToBitmap();
                break;
            }
        }
        catch { }
        _logoRingPanel.Controls.Add(_logo);
        _logoRingPanel.Resize += (_, _) =>
        {
            if (_logoRingPanel == null) return;
            _logo.Left = (_logoRingPanel.ClientSize.Width - _logo.Width) / 2;
            _logo.Top = 22;
        };

        _animTimer = new System.Windows.Forms.Timer { Interval = 50 }; // 20fps — smooth enough, less UI thrash
        _animTimer.Tick += (_, _) =>
        {
            _ringAngle = (_ringAngle + 2.5f) % 360f;
            // Only invalidate the ring band around the logo (not whole sidebar)
            if (_logoRingPanel != null)
            {
                int cx = _logoRingPanel.Width / 2;
                int cy = 50;
                _logoRingPanel.Invalidate(new Rectangle(cx - 40, cy - 40, 80, 80));
            }
        };
        _animTimer.Start();
        StyleBtn(_btnLoad, "Load ELF", Panel2);
        StyleBtn(_btnLift, "Decompile", Panel2);
        StyleBtn(_btnBuild, "Build Project", AccentBlue);
        StyleBtn(_btnRun, "Run Game EXE", Success);
        StyleBtn(_btnCopy, "Copy to Game Folder", Panel2);
        StyleBtn(_btnCancel, "Stop / Cancel Build", Danger);

        _gfx.DropDownStyle = ComboBoxStyle.DropDownList;
        _gfx.Items.AddRange(new object[] { "Vulkan 1.2 (default)", "OpenGL 3.3" });
        _gfx.SelectedIndex = 0;
        _gfx.Enabled = false;
        StyleCombo(_gfx);

        // Threads: ComboBox is reliable; range mirrored for UX
        _threads.DropDownStyle = ComboBoxStyle.DropDownList;
        int maxT = Math.Max(1, Environment.ProcessorCount);
        for (int i = 1; i <= maxT; i++) _threads.Items.Add(i.ToString());
        _threads.SelectedItem = Math.Min(4, maxT).ToString();
        StyleCombo(_threads);

        _eta.ForeColor = TextSec;
        _eta.AutoSize = true;
        _eta.Text = "";
        _eta.Margin = new Padding(0, 4, 0, 0);

        _headerStatus = new Label
        {
            Text = "ENGINE READY",
            ForeColor = Accent,
            BackColor = Color.FromArgb(28, Success),
            Height = 24,
            TextAlign = ContentAlignment.MiddleCenter,
            Dock = DockStyle.Top,
            Font = new Font("Consolas", 8f, FontStyle.Bold),
            Margin = new Padding(0, 10, 0, 0)
        };

        int row = 0;
        void Add(Control c)
        {
            rail.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            rail.Controls.Add(c, 0, row++);
        }

        Add(_logoRingPanel);
        Add(MakeSection("WORKFLOW", "8 ACTIONS"));
        Add(_btnLoad);
        Add(_btnLift);
        Add(_btnBuild);
        Add(_btnCopy);
        Add(_btnRun);
        Add(_btnCancel);
        Add(MakeSection("RECOMPILER SETTINGS", null));
        Add(MakeLabel("GRAPHICS API"));
        Add(_gfx);
        Add(MakeLabel("CPU THREADS"));
        Add(_threads);
        Add(_eta);
        Add(_headerStatus);

        var footerHint = new Label
        {
            Text = "Toolchain: system VS · redist = installer + guide",
            ForeColor = TextSec,
            Font = new Font("Consolas", 7.5f),
            AutoSize = false,
            Height = 28,
            Dock = DockStyle.Top,
            TextAlign = ContentAlignment.MiddleLeft,
            Margin = new Padding(0, 8, 0, 0)
        };
        Add(footerHint);
        sidebar.Controls.Add(rail);

        // ===== RIGHT =====
        var right = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 3,
            BackColor = Bg
        };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 44));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));

        var pathBar = new Panel { Dock = DockStyle.Fill, BackColor = Panel, Padding = new Padding(12, 8, 12, 8) };
        pathBar.Paint += (_, e) =>
        {
            using var pen = new Pen(BorderC);
            e.Graphics.DrawLine(pen, 0, pathBar.Height - 1, pathBar.Width, pathBar.Height - 1);
        };
        _project.Text = "TARGET  /  NO PROJECT LOADED";
        _project.ForeColor = TextPri;
        _project.BackColor = Panel2;
        _project.Dock = DockStyle.Fill;
        pathBar.Controls.Add(_project);

        StyleTextBox(_log);
        StyleTextBox(_report);
StyleTextBox(_roadmap);
        StyleTextBox(_requirements);
        _requirements.Text = BuildRequirementsText();
        _roadmap.Text = BuildRoadmapText();

        _tabs.Dock = DockStyle.Fill;
        _tabs.DrawMode = TabDrawMode.OwnerDrawFixed;
        _tabs.ItemSize = new Size(118, 32);
        _tabs.SizeMode = TabSizeMode.Fixed;
        _tabs.DrawItem += DrawPs3Tab;
        _tabs.BackColor = Bg;
        AddTab("Console", _log);
        AddTab("Report", _report);
        AddTab("Roadmap", _roadmap);
        AddTab("Requirements", _requirements);
        AddTab("Legal", _legalTab);
        BuildControllerTab();

        _status.Text = "Ready";
        _status.TextAlign = ContentAlignment.MiddleLeft;
        _status.Padding = new Padding(10, 0, 0, 0);
        _status.Dock = DockStyle.Fill;
        _status.ForeColor = TextSec;
        _status.BackColor = Panel;
        _progress.Dock = DockStyle.Bottom;
        _progress.Height = 5;
        _progress.Style = ProgressBarStyle.Continuous;

        var statusRow = new Panel { Dock = DockStyle.Fill, BackColor = Panel };
        statusRow.Paint += (_, e) =>
        {
            using var pen = new Pen(BorderC);
            e.Graphics.DrawLine(pen, 0, 0, statusRow.Width, 0);
        };
        statusRow.Controls.Add(_status);
        statusRow.Controls.Add(_progress);

        right.Controls.Add(pathBar, 0, 0);
        right.Controls.Add(_tabs, 0, 1);
        right.Controls.Add(statusRow, 0, 2);

        root.Controls.Add(sidebar, 0, 0);
        root.Controls.Add(right, 1, 0);
        Controls.Add(root);
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling", ct => Native.Lift(_projectDir!, out _), afterLift: true);
        _btnBuild.Click += async (_, _) => await BuildWithProgress();
        _btnRun.Click += async (_, _) => await RunGame();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += async (_, _) =>
        {
            if (_building && !_buildCancelRequested) await ConfirmCancelBuildAsync();
            else StopGame();
        };

        Append("PPSX33 ready · core v1.0.0");
        Append("Settings: " + Settings.DataDir);
        Append("Host build uses system Visual Studio (redist/ = installer + guide only).");
        Append("UI: native WinForms (no WebView2).");
        UpdateButtons();
        Shown += (_, _) =>
        {
            if (_logoRingPanel != null)
            {
                _logo.Left = (_logoRingPanel.ClientSize.Width - _logo.Width) / 2;
                _logo.Top = 22;
            }
        };
        FormClosing += (_, _) =>
        {
            _animTimer?.Stop();
            _animTimer?.Dispose();
            _logoFontTitle?.Dispose();
            _logoFontSub?.Dispose();
            _logoPenDash?.Dispose();
            _logoPenArc?.Dispose();
            CleanupAllProjectWorkFolders();
        };
    }

    void PaintLogoCard(object? sender, PaintEventArgs e)
    {
        var g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias;
        var panel = (Panel)sender!;
        var rect = new Rectangle(6, 6, panel.Width - 12, panel.Height - 12);
        using (var path = RoundRect(rect, 12))
        using (var br = new LinearGradientBrush(rect,
                   Color.FromArgb(40, 0x02, 0x84, 0xC7),
                   Color.FromArgb(50, 0x16, 0x1B, 0x22), 90f))
        {
            g.FillPath(br, path);
            using var pen = new Pen(Color.FromArgb(90, Accent));
            g.DrawPath(pen, path);
        }

        int cx = panel.Width / 2;
        int cy = 50;
        // rotating dashed outer ring
        if (_logoPenDash != null)
        {
            g.TranslateTransform(cx, cy);
            g.RotateTransform(_ringAngle);
            g.DrawEllipse(_logoPenDash, -34, -34, 68, 68);
            g.ResetTransform();
        }
        if (_logoPenArc != null)
        {
            g.TranslateTransform(cx, cy);
            g.RotateTransform(-_ringAngle * 1.4f);
            g.DrawArc(_logoPenArc, -28, -28, 56, 56, 0, 270);
            g.ResetTransform();
        }

        if (_logoFontTitle != null)
            TextRenderer.DrawText(g, "PPSX33", _logoFontTitle,
                new Rectangle(0, 88, panel.Width, 20),
                TextPri, TextFormatFlags.HorizontalCenter);
        if (_logoFontSub != null)
            TextRenderer.DrawText(g, "PS3 BINARY RECOMPILER", _logoFontSub,
                new Rectangle(0, 108, panel.Width, 16),
                Accent, TextFormatFlags.HorizontalCenter);
    }

    static GraphicsPath RoundRect(Rectangle r, int radius)
    {
        int d = radius * 2;
        var p = new GraphicsPath();
        p.AddArc(r.X, r.Y, d, d, 180, 90);
        p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
        p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        p.CloseFigure();
        return p;
    }

    Label MakeSection(string title, string? badge)
    {
        var host = new Label
        {
            Height = 22,
            Dock = DockStyle.Top,
            BackColor = Panel,
            Text = title,
            ForeColor = TextSec,
            Font = new Font("Consolas", 8f, FontStyle.Bold),
            TextAlign = ContentAlignment.MiddleLeft,
            Margin = new Padding(0, 8, 0, 4)
        };
        if (!string.IsNullOrEmpty(badge))
            host.Text = title + "   " + badge;
        return host;
    }

    /// <summary>No-op bridge kept for partials that call PushState after WebView removal.</summary>
    void PushState()
    {
        if (_headerStatus == null) return;
        bool locked = _busy || _building;
        _headerStatus.Text = locked ? "WORKING" : (_controllerMapped ? "MAPPED" : "ENGINE READY");
        _headerStatus.ForeColor = locked ? Color.FromArgb(0xF0, 0xB4, 0x00) : Accent;
    }

    static string BuildRequirementsText() =>
        "PPSX33 — Requirements\r\n" +
        "========================================\r\n\r\n" +
        "Required\r\n" +
        "  OS            64-bit Windows 10/11\r\n" +
        "  Disk          SSD recommended\r\n" +
        "  RAM           8 GB minimum\r\n" +
        "  GPU           Vulkan 1.2 capable GPU + driver\r\n" +
        "  Graphics API  Vulkan 1.2 (default); OpenGL secondary\r\n" +
        "  Toolchain     Visual Studio Community (see redist/README.txt)\r\n\r\n" +
        "Recommended\r\n" +
        "  RAM           16 GB+\r\n" +
        "  GPU           Discrete, 2 GB+ VRAM\r\n" +
        "  Monitor       1920x1080\r\n" +
        "  Future API    OpenGL 3.3 / 2.1 fallback\r\n";

    static string BuildRoadmapText() =>
        "PPSX33 — Roadmap / Live GUI Status\r\n" +
        "========================================\r\n\r\n" +
        "CURRENT GUI WORK\r\n" +
        "  Native C# / WinForms controller UI   IMPLEMENTED\r\n" +
        "  WebView2 controller dependency       REMOVED\r\n" +
        "  Controller HTML asset                REMOVED\r\n" +
        "  Controller body / buttons / D-pad    GDI+ rendered\r\n" +
        "  Stick movement + glow animation      IMPLEMENTED\r\n" +
        "  Key rebinding / Esc cancel            IMPLEMENTED\r\n" +
        "  JSON import / export                  IMPLEMENTED\r\n" +
        "  Save settings + output/pad_map.json   IMPLEMENTED\r\n" +
        "  Compile + visual runtime validation   PENDING LOCAL BUILD\r\n\r\n" +
        "CORE RECOMPILER ROADMAP\r\n" +
        "  I–XXII  Core pipeline                  TESTING / DONE BANDS\r\n" +
        "  XXIII   Execution reality              PARTIAL\r\n" +
        "  XXIV    Boot-to-first-pixels           PARTIAL\r\n" +
        "  XXV     Truthful execution / frames    ACTIVE\r\n" +
        "    A–J   Services / end-to-end checklist\r\n" +
        "    K     Guest color buffer → frames    IN PROGRESS\r\n" +
        "    L     Wild-PC / sticky recovery      INVESTIGATING\r\n" +
        "  XXVI    Host UI polish + deeper RSX    PLANNED / ONGOING\r\n\r\n" +
        "KNOWN VALIDATION GATE\r\n" +
        "  Build the Release GUI locally and fix all compiler errors before marking the UI validated.\r\n" +
        "  Then test resize / DPI, rebind & cancel, import/export, save/reload, and build-lock workflow.\r\n\r\n" +
        "WORKFLOW\r\n" +
        "  Load ELF → Decompile → Build → Run\r\n" +
        "  Repository roadmap: ROADMAP.md and roadmap/*.md\r\n";

    void CleanupAllProjectWorkFolders()
    {
        try
        {
            if (!Directory.Exists(ProjectsRoot)) return;
            foreach (var dir in Directory.GetDirectories(ProjectsRoot))
            {
                try
                {
                    var codebase = Path.Combine(dir, "codebase");
                    var input = Path.Combine(dir, "input");
                    if (Directory.Exists(codebase)) Directory.Delete(codebase, true);
                    if (Directory.Exists(input)) Directory.Delete(input, true);
                }
                catch { }
            }
        }
        catch { }
    }
}
