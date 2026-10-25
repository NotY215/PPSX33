using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

public sealed partial class MainForm : Form
{
    // Colors from test.cs / HTML design system
    static readonly Color Bg       = Color.FromArgb(0x09, 0x0D, 0x14);
    static readonly Color Sidebar  = Color.FromArgb(0x0D, 0x11, 0x17);
    static readonly Color Card     = Color.FromArgb(0x16, 0x1B, 0x22);
    static readonly Color BorderC  = Color.FromArgb(0x1E, 0x29, 0x3B);
    static readonly Color Hover    = Color.FromArgb(0x21, 0x26, 0x2D);
    static readonly Color TextPri  = Color.FromArgb(0xE6, 0xED, 0xF3);
    static readonly Color TextSec  = Color.FromArgb(0x64, 0x74, 0x8B);
    static readonly Color Cyan     = Color.FromArgb(0x38, 0xBD, 0xF8);
    static readonly Color CyanDeep = Color.FromArgb(0x02, 0x84, 0xC7);
    static readonly Color AccentBlue = Color.FromArgb(0x1F, 0x6F, 0xEB);
    static readonly Color Success  = Color.FromArgb(0x23, 0x86, 0x36);
    static readonly Color Danger   = Color.FromArgb(0xDA, 0x36, 0x33);

    // aliases used by partials
    static readonly Color Panel = Sidebar;
    static readonly Color Panel2 = Card;
    static readonly Color Accent = Cyan;
    static readonly Color TextSecAlias = TextSec;

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly TextBox _roadmap = new();
    private readonly TextBox _requirements = new();
    private readonly Ps3HudLabel _project = new();
    private readonly Label _status = new();
    private readonly ProgressBar _progress = new();
    private readonly Ps3Button _btnDecrypt = new();
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
    private string? _projectDir;
    private CancellationTokenSource? _cts;
    private Process? _gameProc;
    private bool _building;
    private bool _buildCancelRequested;
    private bool _canLift, _canBuild, _canCopy, _canRun, _gfxUnlocked, _busy;
    private int _liftChunks;
    private Ps3Button? _btnRpcs3;
    private readonly Label _eta = new();
    private bool _controllerMapped;
    private Label? _headerStatus;

    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "redist"); // guide only; VS is system-installed
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    protected override CreateParams CreateParams
    {
        get
        {
            var cp = base.CreateParams;
            cp.ExStyle |= 0x02000000; // WS_EX_COMPOSITED
            return cp;
        }
    }

    public MainForm()
    {
        Text = "PPSX33 · PlayStation 3 Recompiler";
        StartPosition = FormStartPosition.CenterScreen;
        Size = new Size(1200, 780);
        MinimumSize = new Size(950, 650);
        BackColor = Bg;
        ForeColor = TextPri;
        Font = new Font("Segoe UI", 9f);
        DoubleBuffered = true;

        try
        {
            Load += (_, _) =>
            {
                int useDark = 1;
                DwmSetWindowAttribute(Handle, 20, ref useDark, sizeof(int));
            };
        }
        catch { }

        // ===== ROOT: sidebar | workspace =====
        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            BackColor = Bg,
            Padding = new Padding(0)
        };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 280));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));

        // ----- LEFT SIDEBAR -----
        var sidebar = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Sidebar,
            Padding = new Padding(16),
            AutoScroll = true
        };
        // right border line
        sidebar.Paint += (_, e) =>
        {
            using var pen = new Pen(BorderC, 1);
            e.Graphics.DrawLine(pen, sidebar.Width - 1, 0, sidebar.Width - 1, sidebar.Height);
        };

        var rail = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            ColumnCount = 1,
            BackColor = Sidebar
        };
        rail.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));

        // Logo block
        var logoPanel = new Panel
        {
            Height = 120,
            Dock = DockStyle.Top,
            BackColor = Card,
            Margin = new Padding(0, 0, 0, 16),
            Padding = new Padding(8)
        };
        logoPanel.Paint += (s, e) =>
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            var rect = new Rectangle(8, 8, logoPanel.Width - 16, logoPanel.Height - 16);
            using var path = Rounded(rect, 12);
            using var br = new LinearGradientBrush(rect,
                Color.FromArgb(30, 0x02, 0x84, 0xC7),
                Color.FromArgb(40, 0x16, 0x1B, 0x22), 90f);
            g.FillPath(br, path);
            using var pen = new Pen(Color.FromArgb(80, Cyan), 1);
            g.DrawPath(pen, path);
            // ring
            int cx = logoPanel.Width / 2, cy = 42;
            using (var ring = new Pen(Color.FromArgb(120, CyanDeep), 2))
                g.DrawEllipse(ring, cx - 28, cy - 28, 56, 56);
            using (var ring2 = new Pen(Color.FromArgb(180, Cyan), 1))
                g.DrawEllipse(ring2, cx - 22, cy - 22, 44, 44);
            TextRenderer.DrawText(g, "PPSX33",
                new Font("Segoe UI", 14f, FontStyle.Bold),
                new Rectangle(0, 72, logoPanel.Width, 22),
                TextPri, TextFormatFlags.HorizontalCenter);
            TextRenderer.DrawText(g, "PS3 RECOMPILER",
                new Font("Segoe UI", 7.5f, FontStyle.Bold),
                new Rectangle(0, 94, logoPanel.Width, 16),
                Cyan, TextFormatFlags.HorizontalCenter);
        };

        _logo.Size = new Size(1, 1);
        _logo.Visible = false;
        try
        {
            foreach (var ico in new[]
            {
                Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico"),
                Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico"),
            })
            {
                if (!File.Exists(ico)) continue;
                Icon = new Icon(ico);
                break;
            }
        }
        catch { }

        StyleBtn(_btnDecrypt, "Decrypt EBOOT", Card);
        StyleBtn(_btnLoad, "Load ELF", Card);
        StyleBtn(_btnLift, "Decompile", Card);
        StyleBtn(_btnBuild, "Build Project", AccentBlue);
        StyleBtn(_btnRun, "Run Executable", Success);
        StyleBtn(_btnCopy, "Copy to Game Folder", Card);
        StyleBtn(_btnCancel, "Stop / Cancel Build", Danger);

        _btnRpcs3 = new Ps3Button();
        StyleBtn(_btnRpcs3, "RPCS3 path…", Card);
        _btnRpcs3.Click += (_, _) =>
        {
            using var ofd = new OpenFileDialog
            {
                Title = "Locate RPCS3.exe",
                Filter = "RPCS3|RPCS3.exe|Executables|*.exe|All|*.*"
            };
            if (ofd.ShowDialog(this) != DialogResult.OK) return;
            _settings.Rpcs3Path = ofd.FileName;
            SaveSettings();
            UpdateRpcs3Button();
            Append("RPCS3 path set: " + _settings.Rpcs3Path);
        };

        _gfx.DropDownStyle = ComboBoxStyle.DropDownList;
        _gfx.Items.AddRange(new object[] { "D3D11 (DirectX 11)", "OpenGL 3.3 (future)", "Vulkan (not linked)" });
        _gfx.SelectedIndex = 0;
        _gfx.Enabled = false;
        StyleCombo(_gfx);

        _threads.DropDownStyle = ComboBoxStyle.DropDownList;
        for (int i = 1; i <= Environment.ProcessorCount; i++) _threads.Items.Add(i.ToString());
        _threads.SelectedItem = Math.Min(4, Environment.ProcessorCount).ToString();
        StyleCombo(_threads);

        _eta.ForeColor = TextSec;
        _eta.AutoSize = true;
        _eta.Text = "";

        int r = 0;
        void AddRail(Control c)
        {
            rail.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            rail.Controls.Add(c, 0, r++);
        }

        AddRail(logoPanel);
        AddRail(SectionLabel("WORKFLOW", "7 ACTIONS"));
        AddRail(_btnDecrypt);
        AddRail(_btnLoad);
        AddRail(_btnLift);
        AddRail(_btnBuild);
        AddRail(_btnRun);
        AddRail(_btnCopy);
        AddRail(_btnCancel);
        if (_btnRpcs3 != null) AddRail(_btnRpcs3);
        AddRail(SectionLabel("RECOMPILER SETTINGS", null));
        AddRail(MakeLabel("GRAPHICS API"));
        AddRail(_gfx);
        AddRail(MakeLabel("CPU THREADS"));
        AddRail(_threads);
        AddRail(_eta);

        // header status under settings
        _headerStatus = new Label
        {
            Text = "ENGINE READY",
            ForeColor = Cyan,
            BackColor = Color.FromArgb(20, 0x23, 0x86, 0x36),
            AutoSize = false,
            Height = 22,
            TextAlign = ContentAlignment.MiddleCenter,
            Dock = DockStyle.Top,
            Margin = new Padding(0, 12, 0, 0),
            Font = new Font("Consolas", 8f, FontStyle.Bold)
        };
        AddRail(_headerStatus);

        sidebar.Controls.Add(rail);

        // ----- RIGHT WORKSPACE -----
        var right = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 3,
            BackColor = Bg,
            Padding = new Padding(0)
        };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 44)); // path banner
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f)); // tabs
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 40)); // status

        // Path banner
        var pathBar = new Panel { Dock = DockStyle.Fill, BackColor = Sidebar, Padding = new Padding(12, 8, 12, 8) };
        pathBar.Paint += (_, e) =>
        {
            using var pen = new Pen(BorderC, 1);
            e.Graphics.DrawLine(pen, 0, pathBar.Height - 1, pathBar.Width, pathBar.Height - 1);
        };
        _project.Text = "TARGET  /  NO PROJECT LOADED";
        _project.ForeColor = TextPri;
        _project.BackColor = Card;
        _project.Dock = DockStyle.Fill;
        pathBar.Controls.Add(_project);

        // Tabs + console styling
        StyleTextBox(_log);
        StyleTextBox(_report);
        StyleTextBox(_roadmap);
        StyleTextBox(_requirements);

        _requirements.Text =
            "PPSX33 — Requirements to run a recompiled game\r\n" +
            "========================================\r\n\r\n" +
            "Required\r\n" +
            "  OS            64-bit Windows 10/11\r\n" +
            "  Disk type     SSD recommended\r\n" +
            "  RAM           8 GB minimum\r\n" +
            "  GPU           D3D11 Feature Level 11_0\r\n" +
            "  VRAM          Shared or dedicated\r\n" +
            "  Graphics API  D3D11 (current host path)\r\n" +
            "  Toolchain     Visual Studio Community (see redist/README.txt)\r\n\r\n" +
            "Recommended\r\n" +
            "  RAM           16 GB or more\r\n" +
            "  GPU           Discrete GPU, D3D11 FL11_0\r\n" +
            "  VRAM          2 GB or more dedicated\r\n" +
            "  Monitor       1920x1080\r\n" +
            "  Graphics API  D3D11; future OpenGL 3.3/2.1 fallback\r\n\r\n" +
            "Commercial playability is not guaranteed.\r\n";

        _roadmap.Text =
            "PPSX33 — Architecture Roadmap\r\n" +
            "========================================\r\n\r\n" +
            "PHASE 1 — I–XXII (see roadmap/)\r\n" +
            "  XXII  Pipeline correctness\r\n\r\n" +
            "PHASE XXIII — Execution reality (active)\r\n" +
            "  Guest-driven FIFO, VA/draw path, shader translation\r\n\r\n" +
            "PHASE XXIV+ — Audio HLE, deeper SPU/MFC, compatibility\r\n\r\n" +
            "Workflow: Decrypt/Load ELF → Decompile → Build →\r\n" +
            "Copy to USRDIR → Run from USRDIR.\r\n" +
            "Host toolchain: system Visual Studio (redist is installer+guide only).\r\n";

        _tabs.Dock = DockStyle.Fill;
        _tabs.DrawMode = TabDrawMode.OwnerDrawFixed;
        _tabs.ItemSize = new Size(120, 32);
        _tabs.SizeMode = TabSizeMode.Fixed;
        _tabs.DrawItem += DrawPs3Tab;
        _tabs.BackColor = Bg;
        AddTab("Console", _log);
        AddTab("Report", _report);
        AddTab("Roadmap", _roadmap);
        AddTab("Requirements", _requirements);
        BuildControllerTab();

        _status.TextAlign = ContentAlignment.MiddleLeft;
        _status.Padding = new Padding(10, 0, 0, 0);
        _status.Dock = DockStyle.Fill;
        _status.ForeColor = TextSec;
        _status.BackColor = Sidebar;
        _status.Text = "Ready";
        _progress.Dock = DockStyle.Bottom;
        _progress.Height = 4;
        _progress.Style = ProgressBarStyle.Continuous;

        var statusRow = new Panel { Dock = DockStyle.Fill, BackColor = Sidebar };
        statusRow.Paint += (_, e) =>
        {
            using var pen = new Pen(BorderC, 1);
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

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
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
        UpdateButtons();
        Shown += (_, _) => CheckRpcs3OnStartup();
        FormClosing += (_, _) => CleanupAllProjectWorkFolders();
    }

    static GraphicsPath Rounded(Rectangle r, int radius)
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

    Label SectionLabel(string title, string? badge)
    {
        var p = new Panel { Height = 22, Dock = DockStyle.Top, Margin = new Padding(0, 8, 0, 6), BackColor = Sidebar };
        var t = new Label
        {
            Text = title,
            ForeColor = TextSec,
            Font = new Font("Consolas", 8f, FontStyle.Bold),
            AutoSize = true,
            Location = new Point(0, 2),
            BackColor = Sidebar
        };
        p.Controls.Add(t);
        if (!string.IsNullOrEmpty(badge))
        {
            var b = new Label
            {
                Text = badge,
                ForeColor = TextSec,
                Font = new Font("Consolas", 7f),
                AutoSize = true,
                BackColor = Hover,
                Padding = new Padding(4, 1, 4, 1)
            };
            p.Controls.Add(b);
            p.Resize += (_, _) => { b.Left = Math.Max(t.Right + 8, p.Width - b.Width - 2); b.Top = 2; };
        }
        // SectionLabel returns Label for rail AutoSize — wrap as label height
        var host = new Label { Height = 22, Dock = DockStyle.Top, BackColor = Sidebar, Text = "" };
        host.Controls.Add(p);
        p.Dock = DockStyle.Fill;
        return host;
    }

    /// <summary>On app exit: delete codebase/ and input/ under every projects/* folder; keep output/.</summary>
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
                catch { /* ignore per-project failures */ }
            }
        }
        catch { }
    }
}
