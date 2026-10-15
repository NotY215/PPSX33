using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Drawing.Drawing2D;

namespace PS3Recomp.Gui;

public sealed partial class MainForm : Form
{
    static readonly Color Bg = Color.FromArgb(6, 8, 18);
    static readonly Color Panel = Color.FromArgb(14, 20, 44);
    static readonly Color Panel2 = Color.FromArgb(20, 30, 60);
    static readonly Color TextPri = Color.FromArgb(238, 242, 255);
    static readonly Color TextSec = Color.FromArgb(150, 162, 196);
    static readonly Color Accent = Color.FromArgb(38, 112, 210);
    static readonly Color Danger = Color.FromArgb(210, 55, 78);

    private readonly Settings _settings = Settings.Load();
    private readonly TextBox _log = new();
    private readonly TextBox _report = new();
    private readonly TextBox _roadmap = new();
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
    private bool _controllerHiddenAfterBuild;

    private const int StallQuietMs = 4000;
    private const int MaxRunMs = 0;
    private static string RuntimeDir => Path.Combine(AppContext.BaseDirectory, "runtime");
    private static string CompilersDir => Path.Combine(AppContext.BaseDirectory, "Compilers-files");
    private static string ProjectsRoot => Path.Combine(AppContext.BaseDirectory, "projects");

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int value, int size);

    protected override CreateParams CreateParams
    {
        get
        {
            var cp = base.CreateParams;
            cp.ExStyle |= 0x02000000;
            return cp;
        }
    }

    public MainForm()
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        Text = "PPSX33  ·  PlayStation 3 Recompiler";
        Width = 1180; Height = 760;
        MinimumSize = new Size(980, 660);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Bg; ForeColor = TextPri;
        Font = new Font("Segoe UI", 9.25f);
        DoubleBuffered = true;
        try {
            string ico = Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.ico");
            if (!File.Exists(ico)) ico = Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.ico");
            if (File.Exists(ico)) Icon = new Icon(ico);
        } catch { }

        var backdrop = new Ps3BackdropPanel { Dock = DockStyle.Fill };
        var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Padding = new Padding(16), BackColor = Color.Transparent };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 266));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
        var rail = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(10, 17, 37), Padding = new Padding(14) };
        _logo.SizeMode = PictureBoxSizeMode.Zoom; _logo.Height = 70; _logo.Dock = DockStyle.Top; _logo.BackColor = Color.Transparent;
        try {
            string png = Path.Combine(AppContext.BaseDirectory, "ppsx33-logo.png");
            if (!File.Exists(png)) png = Path.Combine(AppContext.BaseDirectory, "assets", "ppsx33-logo.png");
            if (File.Exists(png)) _logo.Image = Image.FromFile(png);
        } catch { }

        var railTitle = MakeLbl("WORKFLOW", TextSec, true); railTitle.Dock = DockStyle.Top; railTitle.Height = 24;
        StyleBtn(_btnDecrypt, "Decrypt EBOOT", Accent);
        StyleBtn(_btnLoad, "Load ELF", Panel2);
        StyleBtn(_btnLift, "Decompile", Panel2);
        StyleBtn(_btnBuild, "Build", Panel2);
        StyleBtn(_btnRun, "Run", Accent);
        StyleBtn(_btnCopy, "Copy to game folder", Panel2);
        StyleBtn(_btnCancel, "Stop / Cancel", Danger);
        _btnLift.Enabled = _btnBuild.Enabled = _btnRun.Enabled = _btnCopy.Enabled = _btnCancel.Enabled = false;
        var btnStack = new FlowLayoutPanel { Dock = DockStyle.Top, FlowDirection = FlowDirection.TopDown, WrapContents = false, AutoSize = true, BackColor = Color.FromArgb(10, 17, 37), Padding = new Padding(0, 8, 0, 10) };
        foreach (var b in new[] { _btnDecrypt, _btnLoad, _btnLift, _btnBuild, _btnRun, _btnCopy, _btnCancel })
        { b.Width = 224; b.Height = 38; b.Margin = new Padding(0, 0, 0, 7); btnStack.Controls.Add(b); }

        var optsTitle = MakeLbl("OPTIONS", TextSec, true); optsTitle.Dock = DockStyle.Top; optsTitle.Height = 28; optsTitle.Padding = new Padding(0, 8, 0, 0);
        var opts = new Panel { Dock = DockStyle.Top, Height = 170, BackColor = Color.FromArgb(12, 21, 43), Padding = new Padding(6, 0, 0, 0) };
        var gfxLbl = MakeLbl("Graphics backend", TextSec, false); gfxLbl.Location = new Point(0, 4);
        _gfx.DropDownStyle = ComboBoxStyle.DropDownList; _gfx.FlatStyle = FlatStyle.Flat;
        _gfx.BackColor = Panel2; _gfx.ForeColor = TextPri; _gfx.Location = new Point(0, 24); _gfx.Width = 210;
        _gfx.Items.AddRange(new object[] { "D3D11", "D3D10", "Vulkan (exp)" });
        _gfx.SelectedItem = _settings.GraphicsBackend;
        if (_gfx.SelectedIndex < 0) _gfx.SelectedIndex = 0;
        _gfx.SelectedIndexChanged += (_, _) => { _settings.GraphicsBackend = _gfx.SelectedItem?.ToString() ?? "D3D11"; SaveSettings(); };
        var thrLbl = MakeLbl("PPU threads", TextSec, false); thrLbl.Location = new Point(0, 56);
        _threads.DropDownStyle = ComboBoxStyle.DropDownList; _threads.FlatStyle = FlatStyle.Flat;
        _threads.BackColor = Panel2; _threads.ForeColor = TextPri; _threads.Location = new Point(0, 76); _threads.Width = 210;
        int maxT = Math.Clamp(Environment.ProcessorCount, 1, 16);
        for (int i = 1; i <= maxT; i++) _threads.Items.Add(i.ToString());
        _threads.SelectedItem = Math.Clamp(_settings.PpuThreads, 1, maxT).ToString();
        if (_threads.SelectedIndex < 0) _threads.SelectedIndex = 0;
        _threads.SelectedIndexChanged += (_, _) => { if (int.TryParse(_threads.SelectedItem?.ToString(), out int t)) { _settings.PpuThreads = t; SaveSettings(); } };
        _btnRpcs3 = new Ps3Button();
        StyleBtn(_btnRpcs3, "RPCS3 path…", Panel2);
        _btnRpcs3.Location = new Point(0, 112); _btnRpcs3.Width = 210; _btnRpcs3.Height = 32;
        _btnRpcs3.Click += (_, _) => { if (PickRpcs3Exe()) { Append("RPCS3: " + _settings.Rpcs3Path); SaveSettings(); UpdateRpcs3Button(); } };
        opts.Controls.Add(gfxLbl); opts.Controls.Add(_gfx); opts.Controls.Add(thrLbl); opts.Controls.Add(_threads); opts.Controls.Add(_btnRpcs3);
        UpdateRpcs3Button();

        rail.Controls.Add(opts);
        rail.Controls.Add(optsTitle);
        rail.Controls.Add(btnStack);
        rail.Controls.Add(railTitle);
        rail.Controls.Add(_logo);

        var right = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, BackColor = Color.Transparent, Padding = new Padding(12, 0, 0, 0), GrowStyle = TableLayoutPanelGrowStyle.FixedSize };
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        right.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        _project.Text = "PROJECT  /  NO PROJECT LOADED"; _project.ForeColor = TextPri; _project.BackColor = Color.FromArgb(12, 22, 47);
        _project.Dock = DockStyle.Fill; _project.TextAlign = ContentAlignment.MiddleLeft; _project.Padding = new Padding(12, 0, 0, 0);
        StyleTextBox(_log); StyleTextBox(_report); StyleTextBox(_roadmap);
        _roadmap.Text =
            "PPSX33 — Roadmap\r\n" +
            "========================================\r\n\r\n" +
            "EARLIER PHASES (I–XIX) — TESTING\r\n" +
            "  Core recompiler, HLE stubs, RSX host, audio/pad, pipeline\r\n" +
            "  Not final; regressions still possible\r\n\r\n" +
            "PACKAGE (XX-A … XX-G) — DONE\r\n" +
            "  XX-A  Game-named EXE + ps3rt.dll + guest_image + pad_map.json\r\n" +
            "  XX-B  cellFs + early PSARC TOC\r\n" +
            "  XX-C  Copy into PS3_GAME\\USRDIR (auto-find EBOOT.BIN)\r\n" +
            "  XX-D  Controller map → pad_map.json\r\n" +
            "  XX-E  Legal / TERMS first-run (system browser links)\r\n" +
            "  XX-F  Post-build layout (reports at root; output/ runtime)\r\n" +
            "  XX-G  Named run logs; pad log only when window focused\r\n\r\n" +
            "RUNTIME PLAYABILITY (XX-H … XX-O) — DONE IN TREE\r\n" +
            "  XX-H  Null-loop skip + GCM flip/wait HLE\r\n" +
            "  XX-I  Fuller cellGcmSys HLE (display buffers, map, tiles)\r\n" +
            "  XX-J  PSARC TOC + zlib block streaming (needs zlib1.dll)\r\n" +
            "  XX-K  SPU ISA interpreter (LS / arith / branch / channels)\r\n" +
            "  XX-L  MFC list DMA, barriers, atomics, tags\r\n" +
            "  XX-M  SPURS-like job queue + cellSpurs HLE (8 SPU slots)\r\n" +
            "  XX-N  cellAudio → waveOut; GDI status overlay; window title\r\n" +
            "  XX-O  ICON0.PNG window/taskbar icon from PS3_GAME\r\n\r\n" +
            "WORKFLOW\r\n" +
            "  1. Load decrypted ELF (or Decrypt EBOOT via RPCS3)\r\n" +
            "  2. Decompile (lift chunks)\r\n" +
            "  3. Set controller keys (before Build)\r\n" +
            "  4. Build → <GameName>.exe + ps3rt.dll in output/\r\n" +
            "  5. Copy to game folder (USRDIR next to EBOOT.BIN)\r\n" +
            "  6. Run the game-named EXE from USRDIR\r\n\r\n" +
            "STILL EXPERIMENTAL\r\n" +
            "  · Solid blue may persist until real RSX draws / assets load\r\n" +
            "  · SPU ISA is a subset — report stalls with latest log\r\n" +
            "  · See ROADMAP.md on GitHub for files, log tags, symptom guide\r\n\r\n" +
            "PPSX33 is a research toolkit — not a retail port.\r\n";
        _tabs.Dock = DockStyle.Fill; _tabs.DrawMode = TabDrawMode.OwnerDrawFixed; _tabs.ItemSize = new Size(100, 32); _tabs.SizeMode = TabSizeMode.Fixed; _tabs.DrawItem += DrawPs3Tab;
        AddTab("Console", _log); AddTab("Report", _report); AddTab("Roadmap", _roadmap);
        BuildControllerTab();
        _status.TextAlign = ContentAlignment.MiddleLeft; _status.Padding = new Padding(8, 0, 0, 0);
        _status.Dock = DockStyle.Fill; _status.ForeColor = TextSec; _status.BackColor = Color.FromArgb(10, 17, 37);
        _progress.Dock = DockStyle.Bottom; _progress.Height = 6;

        right.Controls.Add(_project, 0, 0);
        right.Controls.Add(_tabs, 0, 1);
        var statusRow = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(10, 17, 37) };
        statusRow.Controls.Add(_status);
        statusRow.Controls.Add(_progress);
        right.Controls.Add(statusRow, 0, 2);

        root.Controls.Add(rail, 0, 0);
        root.Controls.Add(right, 1, 0);
        backdrop.Controls.Add(root);
        Controls.Add(backdrop);

        _btnDecrypt.Click += async (_, _) => await DecryptEbootAndLoad();
        _btnLoad.Click += (_, _) => LoadElf(null);
        _btnLift.Click += async (_, _) => await RunStep("Decompiling…", ct => Native.Lift(_projectDir!, out _), afterLift: true);
        _btnBuild.Click += async (_, _) => await BuildWithProgress();
        _btnRun.Click += async (_, _) => await RunGame();
        _btnCopy.Click += (_, _) => CopyToGameFolder();
        _btnCancel.Click += async (_, _) => {
            if (_building && !_buildCancelRequested) await ConfirmCancelBuildAsync();
            else StopGame();
        };

        Append("PPSX33 ready · core v1.0.0");
        Append("Settings: " + Settings.DataDir);
        UpdateButtons();
        // After UI is up: validate RPCS3 path; show button + popup if missing
        Shown += (_, _) => CheckRpcs3OnStartup();
    }
}
